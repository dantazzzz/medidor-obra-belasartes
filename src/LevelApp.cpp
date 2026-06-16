// ============================================================================
//  LevelApp.cpp  -  tela de uma ferramenta de medicao na tela redonda 412x412
// ----------------------------------------------------------------------------
//  Modos: 0 NIVEL  1 PRUMO  2 DECLIVIDADE  3 TRANSFERIDOR  4 RUIDO(dB)
//  O modo e escolhido no MENU (swipe). Aqui temos: BACK (volta ao menu),
//  ZERAR (referencia=0 / reseta min-max), HOLD (congela), CAPTURAR (salva no log).
//  Texto na tela e ASCII (as fontes Montserrat embutidas nao tem acento/grau).
// ============================================================================
#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "LevelApp.h"
#include "Mic_dB.h"
#include "DataLog.h"

extern void AppUi_ShowMenu();      // volta para o menu (definido em AppUi.cpp)

#define DEG (57.29577951f)
#define PXDEG     3.0f             // px por grau (menor = bolha menos sensivel)
#define BUB_LIM   120.0f           // limite de deslocamento da bolha (px)
#define LEVEL_TOL 0.4f             // tolerancia de "nivelado" (graus)
#define SMOOTH    0.12f            // filtro (menor = mais suave/estavel)
#define SIGN_X    (+1.0f)
#define SIGN_Y    (-1.0f)

// --- Objetos da UI ----------------------------------------------------------
static lv_obj_t *root;
static lv_obj_t *bubble, *ringOuter, *ringTol, *hLine, *vLine;
static lv_obj_t *lblMode, *lblBig, *lblUnit, *lblStatus, *lblHint, *lblNorm;

// --- Estado -----------------------------------------------------------------
static int   g_mode = 0;
static bool  g_hold = false;
static float roll0 = 0, pitch0 = 0;
static float fRoll = 0, fPitch = 0;
static float curR = 0, curP = 0;
static float heldR = 0, heldP = 0;
static float dbMin = 999, dbMax = 0;
static float g_lastBig = 0;             // ultimo valor exibido (p/ CAPTURAR)
static char  g_lastUnit[6] = "GRAUS";
static int   g_saved = 0;               // contador p/ mostrar "SALVO"

static const char *MODE_NAMES[5] =
    {"NIVEL", "PRUMO", "DECLIVIDADE", "TRANSFERIDOR", "RUIDO"};

// Presets de norma p/ DECLIVIDADE (% ; valor < 0 = sem limite). Guia, confira a norma.
struct Norm { const char *name; float mn; float mx; };
static const Norm NORMS[] = {
    {"RAMPA NBR9050  ate 8.3%", -1.0f, 8.33f},
    {"PISO p/ RALO  0.5-2%",     0.5f, 2.0f},
    {"ESGOTO 100mm  min 1%",     1.0f, -1.0f},
    {"AGUA PLUVIAL  min 0.5%",   0.5f, -1.0f},
    {"LAJE IMPERM.  min 1%",     1.0f, -1.0f},
    {"LIVRE (sem limite)",      -1.0f, -1.0f},
};
static const int NNORM = sizeof(NORMS) / sizeof(NORMS[0]);
static int g_norm = 0;

static void setHidden(lv_obj_t *o, bool h) {
    if (h) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else   lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

// ----------------------------------------------------------------------------
//  Callbacks dos botoes
// ----------------------------------------------------------------------------
static void onBack(lv_event_t *e)  { (void)e; AppUi_ShowMenu(); }
static void onZero(lv_event_t *e) {
    (void)e;
    if (g_mode == 4) { dbMin = 999; dbMax = 0; }
    else             { roll0 = fRoll; pitch0 = fPitch; }
    g_hold = false;
}
static void onHold(lv_event_t *e) {
    (void)e;
    g_hold = !g_hold;
    if (g_hold) { heldR = curR; heldP = curP; }
}
static void onCapture(lv_event_t *e) {
    (void)e;
    DataLog_Add(MODE_NAMES[g_mode], g_lastBig, g_lastUnit);
    g_saved = 60;                       // mostra "SALVO" por ~1s
}
static void onNorm(lv_event_t *e)  { (void)e; g_norm = (g_norm + 1) % NNORM; }

// helper: botao arredondado com rotulo
static lv_obj_t *mkBtn(lv_obj_t *parent, const char *txt, lv_align_t al,
                       int x, int y, int w, lv_event_cb_t cb, uint32_t color) {
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, w, 44);
    lv_obj_align(b, al, x, y);
    lv_obj_set_style_radius(b, 22, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(color), 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    return b;
}

// ----------------------------------------------------------------------------
//  Monta a tela (dentro do container 'parent')
// ----------------------------------------------------------------------------
void LevelApp_Build(lv_obj_t *parent) {
    root = parent;
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    ringOuter = lv_obj_create(root);
    lv_obj_set_size(ringOuter, 392, 392);
    lv_obj_center(ringOuter);
    lv_obj_clear_flag(ringOuter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ringOuter, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ringOuter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ringOuter, 2, 0);
    lv_obj_set_style_border_color(ringOuter, lv_color_hex(0x3a4452), 0);

    hLine = lv_obj_create(root);
    lv_obj_set_size(hLine, 300, 2);
    lv_obj_center(hLine);
    lv_obj_set_style_bg_color(hLine, lv_color_hex(0x2a313a), 0);
    lv_obj_set_style_border_width(hLine, 0, 0);
    lv_obj_clear_flag(hLine, LV_OBJ_FLAG_SCROLLABLE);
    vLine = lv_obj_create(root);
    lv_obj_set_size(vLine, 2, 300);
    lv_obj_center(vLine);
    lv_obj_set_style_bg_color(vLine, lv_color_hex(0x2a313a), 0);
    lv_obj_set_style_border_width(vLine, 0, 0);
    lv_obj_clear_flag(vLine, LV_OBJ_FLAG_SCROLLABLE);

    ringTol = lv_obj_create(root);
    lv_obj_set_size(ringTol, 70, 70);
    lv_obj_center(ringTol);
    lv_obj_clear_flag(ringTol, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ringTol, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ringTol, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ringTol, 2, 0);
    lv_obj_set_style_border_color(ringTol, lv_color_hex(0x2f6b43), 0);

    bubble = lv_obj_create(root);
    lv_obj_set_size(bubble, 48, 48);
    lv_obj_center(bubble);
    lv_obj_clear_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bubble, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(bubble, lv_color_hex(0x22c55e), 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bubble, 2, 0);
    lv_obj_set_style_border_color(bubble, lv_color_hex(0x0b3d22), 0);

    lblMode = lv_label_create(root);
    lv_obj_set_style_text_font(lblMode, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lblMode, lv_color_hex(0x9aa7b4), 0);
    lv_obj_align(lblMode, LV_ALIGN_TOP_MID, 0, 56);
    lv_label_set_text(lblMode, MODE_NAMES[0]);

    lblBig = lv_label_create(root);
    lv_obj_set_style_text_font(lblBig, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(lblBig, lv_color_hex(0xffffff), 0);
    lv_obj_align(lblBig, LV_ALIGN_TOP_MID, 0, 86);
    lv_label_set_text(lblBig, "0.0");

    lblUnit = lv_label_create(root);
    lv_obj_set_style_text_font(lblUnit, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lblUnit, lv_color_hex(0x9aa7b4), 0);
    lv_obj_align(lblUnit, LV_ALIGN_TOP_MID, 0, 138);
    lv_label_set_text(lblUnit, "GRAUS");

    lblStatus = lv_label_create(root);
    lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x22c55e), 0);
    lv_obj_align(lblStatus, LV_ALIGN_CENTER, 0, 52);
    lv_label_set_text(lblStatus, "");

    lblHint = lv_label_create(root);
    lv_obj_set_style_text_font(lblHint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblHint, lv_color_hex(0x5b6673), 0);
    lv_obj_align(lblHint, LV_ALIGN_CENTER, 0, 78);
    lv_label_set_text(lblHint, "");

    lblNorm = lv_label_create(root);
    lv_obj_set_width(lblNorm, 320);
    lv_obj_set_style_text_font(lblNorm, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblNorm, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_align(lblNorm, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_ver(lblNorm, 8, 0);
    lv_obj_align(lblNorm, LV_ALIGN_TOP_MID, 0, 166);
    lv_obj_add_flag(lblNorm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lblNorm, onNorm, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lblNorm, "");
    lv_obj_add_flag(lblNorm, LV_OBJ_FLAG_HIDDEN);

    // 4 botoes numa linha DENTRO do circulo (a parte de baixo da tela redonda
    // corta, entao nada de BOTTOM_MID nem cantos). MENU volta pro menu.
    mkBtn(root, "MENU",   LV_ALIGN_CENTER, -105, 108, 64, onBack,    0x374151);
    mkBtn(root, "ZERAR",  LV_ALIGN_CENTER,  -35, 108, 64, onZero,    0x2563eb);
    mkBtn(root, "HOLD",   LV_ALIGN_CENTER,   35, 108, 64, onHold,    0x1f2937);
    mkBtn(root, "SALVAR", LV_ALIGN_CENTER,  105, 108, 64, onCapture, 0x16a34a);
}

void LevelApp_SetMode(int mode) {
    g_mode = mode;
    g_hold = false;
    if (mode == 4) { dbMin = 999; dbMax = 0; }
}

const char *LevelApp_ModeName() { return MODE_NAMES[g_mode]; }
float       LevelApp_Value()    { return g_lastBig; }
const char *LevelApp_Unit()     { return g_lastUnit; }

// ----------------------------------------------------------------------------
//  Atualizacao (no loop, quando a ferramenta esta ativa)
// ----------------------------------------------------------------------------
void LevelApp_Update(float ax, float ay, float az) {
    float roll  = atan2f(ay, az) * DEG;
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * DEG;

    fRoll  += SMOOTH * (roll  - fRoll);
    fPitch += SMOOTH * (pitch - fPitch);

    float r = fRoll  - roll0;
    float p = fPitch - pitch0;
    curR = r; curP = p;
    if (g_hold) { r = heldR; p = heldP; }

    float bx = 0, by = 0, big = 0;
    const char *unit = "GRAUS";
    bool levelOk = false;
    bool noiseMode = (g_mode == 4);
    switch (g_mode) {
        case 0: bx = r; by = p; big = sqrtf(r * r + p * p); levelOk = big < LEVEL_TOL; break;
        case 1: bx = 0; by = p; big = fabsf(p);             levelOk = fabsf(p) < LEVEL_TOL; break;
        case 2: bx = 0; by = p; big = tanf(p / DEG) * 100.0f; unit = "%"; break;
        case 3: bx = r; by = p; big = sqrtf(r * r + p * p); break;
        case 4: { float db = MicDB_Get();
                  if (db < dbMin) dbMin = db;
                  if (db > dbMax) dbMax = db;
                  big = db; unit = "dB"; break; }
    }
    g_lastBig = big;
    strncpy(g_lastUnit, unit, sizeof(g_lastUnit) - 1); g_lastUnit[sizeof(g_lastUnit) - 1] = 0;

    setHidden(bubble,    noiseMode);
    setHidden(ringOuter, noiseMode);
    setHidden(hLine,     noiseMode);
    setHidden(vLine,     noiseMode);
    setHidden(lblNorm,   g_mode != 2);

    if (!noiseMode) {
        float ox = SIGN_X * bx * PXDEG;
        float oy = SIGN_Y * by * PXDEG;
        if (ox >  BUB_LIM) ox =  BUB_LIM; if (ox < -BUB_LIM) ox = -BUB_LIM;
        if (oy >  BUB_LIM) oy =  BUB_LIM; if (oy < -BUB_LIM) oy = -BUB_LIM;
        lv_obj_align(bubble, LV_ALIGN_CENTER, (int)ox, (int)oy);
    }
    lv_obj_set_style_border_opa(ringTol,
        (g_mode == 0 || g_mode == 1) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

    char vbuf[16];
    snprintf(vbuf, sizeof(vbuf), "%.1f", (double)big);
    lv_label_set_text(lblMode, MODE_NAMES[g_mode]);
    lv_label_set_text(lblBig, vbuf);
    lv_label_set_text(lblUnit, unit);

    // mensagem "SALVO" tem prioridade na dica
    bool showSaved = (g_saved > 0);
    if (g_saved > 0) g_saved--;

    if (g_mode == 4) {
        uint32_t c; const char *s;
        if (big < 70)      { c = 0x22c55e; s = "OK"; }
        else if (big < 85) { c = 0xf59e0b; s = "ATENCAO"; }
        else               { c = 0xef4444; s = "ALTO (NR-15)"; }
        lv_obj_set_style_text_color(lblBig, lv_color_hex(c), 0);
        lv_obj_set_style_text_color(lblStatus, lv_color_hex(c), 0);
        lv_label_set_text(lblStatus, s);
        char hbuf[40];
        snprintf(hbuf, sizeof(hbuf), "min %.0f   max %.0f",
                 (double)(dbMin > 900 ? big : dbMin), (double)dbMax);
        lv_label_set_text(lblHint, showSaved ? "SALVO" : hbuf);
    }
    else if (g_mode == 2) {
        const Norm &nm = NORMS[g_norm];
        float v = fabsf(big);
        bool livre = (nm.mn < 0 && nm.mx < 0);
        bool ok = (nm.mn < 0 || v >= nm.mn) && (nm.mx < 0 || v <= nm.mx);
        lv_label_set_text(lblNorm, nm.name);
        if (livre) {
            lv_obj_set_style_text_color(lblBig, lv_color_hex(0xffffff), 0);
            lv_label_set_text(lblStatus, "");
        } else if (ok) {
            lv_obj_set_style_text_color(lblBig, lv_color_hex(0x22c55e), 0);
            lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x22c55e), 0);
            lv_label_set_text(lblStatus, "DENTRO DA NORMA");
        } else {
            lv_obj_set_style_text_color(lblBig, lv_color_hex(0xef4444), 0);
            lv_obj_set_style_text_color(lblStatus, lv_color_hex(0xef4444), 0);
            lv_label_set_text(lblStatus, "FORA");
        }
        lv_label_set_text(lblHint, showSaved ? "SALVO" : (g_hold ? "[ congelado ]" : ""));
    }
    else {
        lv_obj_set_style_text_color(lblBig, lv_color_hex(0xffffff), 0);
        if (levelOk) {
            lv_obj_set_style_bg_color(bubble, lv_color_hex(0x4ade80), 0);
            lv_obj_set_style_text_color(lblStatus, lv_color_hex(0x22c55e), 0);
            lv_label_set_text(lblStatus, (g_mode == 1) ? "PRUMO OK" : "NIVELADO");
        } else {
            lv_obj_set_style_bg_color(bubble, lv_color_hex(0x22c55e), 0);
            lv_label_set_text(lblStatus, "");
        }
        lv_label_set_text(lblHint, showSaved ? "SALVO" : (g_hold ? "[ congelado ]" : ""));
    }
}
