// ============================================================================
//  AppUi.cpp  -  telas: SPLASH (logo Belas Artes) -> MENU (carrossel com swipe)
//  -> FERRAMENTA (LevelApp) -> volta ao MENU. Tudo em LVGL 8.
// ============================================================================
#include <lvgl.h>
#include <Arduino.h>
#include <stdint.h>
#include "AppUi.h"
#include "LevelApp.h"
#include "WebPortal.h"
#include "SunApp.h"
#include "Mic_dB.h"
#include "Display_SPD2010.h"

enum { ST_SPLASH, ST_MENU, ST_TOOL, ST_DATA, ST_SUN, ST_SET };
static int      st = ST_SPLASH;
static uint32_t splashStart = 0;

static lv_obj_t *splashCont, *menuCont, *toolCont, *dataCont, *sunCont, *setCont;

#define NCARDS 11
// Cartoes do menu (8 ferramentas + SOL + DADOS + AJUSTES)
static const char *CARD_NAME[NCARDS] =
    {"NIVEL", "PRUMO", "DECLIVIDADE", "TRANSFERIDOR", "RUIDO",
     "CONVERSOR", "ESQUADRO", "PLANEZA", "SOL", "DADOS", "AJUSTES"};
static const char *CARD_SUB[NCARDS] =
    {"bolha 2D", "verticalidade", "caimento %", "angulo", "decibelimetro",
     "graus/%/mm-m", "quina 90", "planeza/empeno", "carta solar", "WiFi + celular", "calibrar/brilho"};
static const uint32_t CARD_COL[NCARDS] =
    {0x2563eb, 0x7c3aed, 0x0891b2, 0xca8a04, 0xdc2626,
     0x0d9488, 0x65a30d, 0x9333ea, 0xb45309, 0x0f766e, 0x64748b};
static lv_obj_t *dotObjs[NCARDS];   // indicador de paginas do menu

static void vis(lv_obj_t *o, bool on) {
    if (!o) return;
    if (on) lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
    else    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}
static void show(int state) {
    st = state;
    vis(splashCont, state == ST_SPLASH);
    vis(menuCont,   state == ST_MENU);
    vis(toolCont,   state == ST_TOOL);
    vis(dataCont,   state == ST_DATA);
    vis(sunCont,    state == ST_SUN);
    vis(setCont,    state == ST_SET);
}

static void onCard(lv_event_t *e) {
    lv_obj_t *card = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(card);
    if      (idx == 8)  AppUi_ShowSun();
    else if (idx == 9)  AppUi_ShowData();
    else if (idx == 10) AppUi_ShowSettings();
    else                AppUi_OpenTool(idx);     // 0..7 abrem ferramentas
}

// destaca o pontinho do cartao que esta centralizado (segue o swipe)
static void onScroll(lv_event_t *e) {
    lv_obj_t *c = lv_event_get_target(e);
    lv_area_t a; lv_obj_get_coords(c, &a);
    int mid = (a.x1 + a.x2) / 2, best = 0, bestd = 1 << 30;
    uint32_t n = lv_obj_get_child_cnt(c);
    for (uint32_t i = 0; i < n; i++) {
        lv_area_t ca; lv_obj_get_coords(lv_obj_get_child(c, i), &ca);
        int cc = (ca.x1 + ca.x2) / 2, d = (cc > mid) ? (cc - mid) : (mid - cc);
        if (d < bestd) { bestd = d; best = (int)i; }
    }
    for (int i = 0; i < NCARDS; i++) {
        lv_obj_set_size(dotObjs[i], i == best ? 16 : 7, 7);
        lv_obj_set_style_bg_color(dotObjs[i], lv_color_hex(i == best ? 0xe2e8f0 : 0x374151), 0);
    }
}

// ---- SPLASH ---------------------------------------------------------------
extern "C" const lv_img_dsc_t logo_belasartes;   // gerado em logo_belasartes.c

static void buildSplash(lv_obj_t *scr) {
    splashCont = lv_obj_create(scr);
    lv_obj_set_size(splashCont, 412, 412);
    lv_obj_center(splashCont);
    lv_obj_clear_flag(splashCont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(splashCont, 0, 0);
    lv_obj_set_style_border_width(splashCont, 0, 0);
    lv_obj_set_style_bg_color(splashCont, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(splashCont, LV_OPA_COVER, 0);

    lv_obj_t *img = lv_img_create(splashCont);
    lv_img_set_src(img, &logo_belasartes);
    lv_obj_set_style_img_recolor(img, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, -36);

    lv_obj_t *a = lv_label_create(splashCont);
    lv_obj_set_style_text_font(a, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(a, lv_color_hex(0xffffff), 0);
    lv_label_set_text(a, "BELAS ARTES");
    lv_obj_align(a, LV_ALIGN_CENTER, 0, 112);

    lv_obj_t *c = lv_label_create(splashCont);
    lv_obj_set_style_text_font(c, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(c, lv_color_hex(0x38bdf8), 0);
    lv_label_set_text(c, "Medidor de Obra");
    lv_obj_align(c, LV_ALIGN_CENTER, 0, 146);
}

// ---- MENU (carrossel com swipe) -------------------------------------------
static void buildMenu(lv_obj_t *scr) {
    menuCont = lv_obj_create(scr);
    lv_obj_set_size(menuCont, 412, 412);
    lv_obj_center(menuCont);
    lv_obj_clear_flag(menuCont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(menuCont, 0, 0);
    lv_obj_set_style_border_width(menuCont, 0, 0);
    lv_obj_set_style_bg_color(menuCont, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(menuCont, LV_OPA_COVER, 0);

    // cabecalho minimalista
    lv_obj_t *title = lv_label_create(menuCont);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xf1f5f9), 0);
    lv_label_set_text(title, "Medidor de Obra");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 34);

    lv_obj_t *sub = lv_label_create(menuCont);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x475569), 0);
    lv_label_set_text(sub, "Belas Artes");
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 62);

    // carrossel: linha de cartoes com snap no centro
    lv_obj_t *car = lv_obj_create(menuCont);
    lv_obj_set_size(car, 412, 250);
    lv_obj_align(car, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_bg_opa(car, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(car, 0, 0);
    lv_obj_set_flex_flow(car, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(car, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_snap_x(car, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_dir(car, LV_DIR_HOR);
    lv_obj_add_flag(car, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scrollbar_mode(car, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_left(car, 76, 0);
    lv_obj_set_style_pad_right(car, 76, 0);
    lv_obj_set_style_pad_column(car, 16, 0);
    lv_obj_add_event_cb(car, onScroll, LV_EVENT_SCROLL, NULL);

    for (int i = 0; i < NCARDS; i++) {
        lv_obj_t *card = lv_obj_create(car);
        lv_obj_set_size(card, 258, 224);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(card, 26, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x0e131c), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x1e293b), 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_user_data(card, (void *)(intptr_t)i);
        lv_obj_add_event_cb(card, onCard, LV_EVENT_CLICKED, NULL);

        lv_obj_t *acc = lv_obj_create(card);            // acento de cor (minimalista)
        lv_obj_set_size(acc, 46, 5);
        lv_obj_align(acc, LV_ALIGN_TOP_MID, 0, 44);
        lv_obj_clear_flag(acc, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(acc, 3, 0);
        lv_obj_set_style_border_width(acc, 0, 0);
        lv_obj_set_style_bg_color(acc, lv_color_hex(CARD_COL[i]), 0);

        lv_obj_t *n = lv_label_create(card);
        lv_obj_set_style_text_font(n, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(n, lv_color_hex(0xf1f5f9), 0);
        lv_label_set_text(n, CARD_NAME[i]);
        lv_obj_align(n, LV_ALIGN_CENTER, 0, -4);

        lv_obj_t *s = lv_label_create(card);
        lv_obj_set_style_text_font(s, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s, lv_color_hex(0x64748b), 0);
        lv_label_set_text(s, CARD_SUB[i]);
        lv_obj_align(s, LV_ALIGN_CENTER, 0, 28);
    }

    // indicador de paginas (pontinhos que seguem o swipe)
    lv_obj_t *dots = lv_obj_create(menuCont);
    lv_obj_set_size(dots, 412, 18);
    lv_obj_align(dots, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_set_style_bg_opa(dots, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots, 0, 0);
    lv_obj_clear_flag(dots, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots, 6, 0);
    for (int i = 0; i < NCARDS; i++) {
        lv_obj_t *d = lv_obj_create(dots);
        lv_obj_set_size(d, i == 0 ? 16 : 7, 7);
        lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(d, 4, 0);
        lv_obj_set_style_border_width(d, 0, 0);
        lv_obj_set_style_bg_color(d, lv_color_hex(i == 0 ? 0xe2e8f0 : 0x374151), 0);
        dotObjs[i] = d;
    }
}

// ---- DADOS / WiFi ---------------------------------------------------------
static void onDataBack(lv_event_t *e) { (void)e; AppUi_ShowMenu(); }

static void buildData(lv_obj_t *scr) {
    dataCont = lv_obj_create(scr);
    lv_obj_set_size(dataCont, 412, 412);
    lv_obj_center(dataCont);
    lv_obj_clear_flag(dataCont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(dataCont, 0, 0);
    lv_obj_set_style_border_width(dataCont, 0, 0);
    lv_obj_set_style_bg_color(dataCont, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(dataCont, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(dataCont);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x2dd4bf), 0);
    lv_label_set_text(title, "DADOS / WiFi");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *info = lv_label_create(dataCont);
    lv_obj_set_width(info, 300);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info, lv_color_hex(0xe5e7eb), 0);
    lv_label_set_text(info,
        "1) No celular, conecte no WiFi:\n"
        "rede: " AP_SSID "\nsenha: " AP_PASS "\n\n"
        "2) abra no navegador:\nhttp://" AP_URL "\n\n"
        "Veja e baixe (CSV) as medicoes.");
    lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *b = lv_btn_create(dataCont);
    lv_obj_set_size(b, 100, 44);
    lv_obj_align(b, LV_ALIGN_CENTER, 0, 122);
    lv_obj_set_style_radius(b, 22, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x374151), 0);
    lv_obj_add_event_cb(b, onDataBack, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, "MENU");
    lv_obj_center(l);
}

// ---- AJUSTES (configuracoes) ----------------------------------------------
static lv_obj_t *s_lblBri, *s_lblRef, *s_lblFlip, *s_lblBeep;
static int g_bright = 60;

static void setRefresh() {
    char b[48];
    snprintf(b, sizeof(b), "Brilho:  %d%%", g_bright);                          lv_label_set_text(s_lblBri, b);
    snprintf(b, sizeof(b), "Ruido (ref dB):  %.0f", (double)MicDB_GetRef());     lv_label_set_text(s_lblRef, b);
    snprintf(b, sizeof(b), "Bolha:   X %c    Y %c",
             LevelApp_SignXPos() ? '+' : '-', LevelApp_SignYPos() ? '+' : '-');  lv_label_set_text(s_lblFlip, b);
    snprintf(b, sizeof(b), "Bip de nivel:  %s", LevelApp_GetBeep() ? "LIGADO" : "DESLIGADO"); lv_label_set_text(s_lblBeep, b);
}

static void onBriM (lv_event_t *e){ (void)e; g_bright -= 10; if (g_bright < 10)  g_bright = 10;  Set_Backlight(g_bright); setRefresh(); }
static void onBriP (lv_event_t *e){ (void)e; g_bright += 10; if (g_bright > 100) g_bright = 100; Set_Backlight(g_bright); setRefresh(); }
static void onRefM (lv_event_t *e){ (void)e; MicDB_SetRef(MicDB_GetRef() - 1); setRefresh(); }
static void onRefP (lv_event_t *e){ (void)e; MicDB_SetRef(MicDB_GetRef() + 1); setRefresh(); }
static void onFlipX(lv_event_t *e){ (void)e; LevelApp_FlipX(); setRefresh(); }
static void onFlipY(lv_event_t *e){ (void)e; LevelApp_FlipY(); setRefresh(); }
static void onBeepT(lv_event_t *e){ (void)e; LevelApp_SetBeep(!LevelApp_GetBeep()); setRefresh(); }
static void onSetBack(lv_event_t *e){ (void)e; AppUi_ShowMenu(); }

static lv_obj_t *sBtn(lv_obj_t *p, const char *txt, int x, int y, int w, lv_event_cb_t cb, uint32_t col) {
    lv_obj_t *b = lv_btn_create(p);
    lv_obj_set_size(b, w, 36);
    lv_obj_align(b, LV_ALIGN_TOP_MID, x, y);
    lv_obj_set_style_radius(b, 18, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(col), 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l = lv_label_create(b); lv_label_set_text(l, txt); lv_obj_center(l);
    return b;
}
static lv_obj_t *sLabel(lv_obj_t *p, int y) {
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xe5e7eb), 0);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
    return l;
}

static void buildSettings(lv_obj_t *scr) {
    setCont = lv_obj_create(scr);
    lv_obj_set_size(setCont, 412, 412);
    lv_obj_center(setCont);
    lv_obj_clear_flag(setCont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(setCont, 0, 0);
    lv_obj_set_style_border_width(setCont, 0, 0);
    lv_obj_set_style_bg_color(setCont, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(setCont, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(setCont);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x94a3b8), 0);
    lv_label_set_text(title, "AJUSTES");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    s_lblBri = sLabel(setCont, 78);                            // Brilho
    sBtn(setCont, "-", -120, 72, 44, onBriM, 0x374151);
    sBtn(setCont, "+",  120, 72, 44, onBriP, 0x374151);

    s_lblRef = sLabel(setCont, 132);                           // Ruido (calibracao dB)
    sBtn(setCont, "-", -120, 126, 44, onRefM, 0x374151);
    sBtn(setCont, "+",  120, 126, 44, onRefP, 0x374151);

    s_lblFlip = sLabel(setCont, 184);                          // Inverter bolha
    sBtn(setCont, "inv X", -68, 200, 72, onFlipX, 0x2563eb);
    sBtn(setCont, "inv Y",  68, 200, 72, onFlipY, 0x2563eb);

    s_lblBeep = sLabel(setCont, 250);                          // Bip
    sBtn(setCont, "trocar", 0, 266, 96, onBeepT, 0x16a34a);

    sBtn(setCont, "MENU", 0, 322, 104, onSetBack, 0x475569);   // Voltar

    setRefresh();
}

// ---- API ------------------------------------------------------------------
void AppUi_Init() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    buildSplash(scr);
    buildMenu(scr);

    toolCont = lv_obj_create(scr);
    lv_obj_set_size(toolCont, 412, 412);
    lv_obj_center(toolCont);
    lv_obj_set_style_radius(toolCont, 0, 0);
    lv_obj_set_style_border_width(toolCont, 0, 0);
    lv_obj_set_style_pad_all(toolCont, 0, 0);
    LevelApp_Build(toolCont);

    buildData(scr);

    sunCont = lv_obj_create(scr);
    lv_obj_set_size(sunCont, 412, 412);
    lv_obj_center(sunCont);
    lv_obj_set_style_radius(sunCont, 0, 0);
    lv_obj_set_style_border_width(sunCont, 0, 0);
    lv_obj_set_style_pad_all(sunCont, 0, 0);
    SunApp_Build(sunCont);

    buildSettings(scr);

    splashStart = millis();
    show(ST_SPLASH);
}

void AppUi_Loop() {
    if (st == ST_SPLASH && (millis() - splashStart) > 2500) {
        show(ST_MENU);
    }
}

void AppUi_OpenTool(int mode) {
    LevelApp_SetMode(mode);
    show(ST_TOOL);
}

void AppUi_ShowMenu()     { show(ST_MENU); }
void AppUi_ShowData()     { show(ST_DATA); }
void AppUi_ShowSun()      { show(ST_SUN); }
void AppUi_ShowSettings() { show(ST_SET); }
bool AppUi_ToolActive()   { return st == ST_TOOL; }
bool AppUi_SunActive()    { return st == ST_SUN; }
