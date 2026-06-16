// ============================================================================
//  SunApp.cpp  -  carta solar (cupula do ceu) desenhada na tela redonda.
//  Centro = zenite (sol a pino), borda = horizonte. Caminho do sol no dia +
//  posicao do sol agora (pelo relogio) + direcao da fachada + numeros.
// ============================================================================
#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include "SunApp.h"
#include "Sun.h"
#include "RTC_PCF85063.h"

extern void AppUi_ShowMenu();

#define CX    206
#define CY    150
#define RMAX  96
#define RADf  0.01745329252f

static lv_obj_t *horizon, *ring45, *pathLine, *facLine, *sunDot;
static lv_obj_t *lblA, *lblB, *lblNow;
static lv_point_t pathPts[48], facPts[2];
static float pathAz[48], pathEl[48];
static int   throttle = 0;

static void mapPolar(float az, float el, int *x, int *y) {
    float r = (90.0f - el) / 90.0f * RMAX;       // zenite no centro, horizonte na borda
    *x = CX + (int)(r * sinf(az * RADf));
    *y = CY - (int)(r * cosf(az * RADf));         // Norte p/ cima, Leste p/ direita
}
static void hm(float h, char *buf, int n) {
    h = fmodf(h + 24.0f, 24.0f);
    int t = (int)(h * 60.0f + 0.5f);
    snprintf(buf, n, "%02d:%02d", (t / 60) % 24, t % 60);
}

static void onBack(lv_event_t *e) { (void)e; AppUi_ShowMenu(); }

static lv_obj_t *ringObj(lv_obj_t *p, int rad, uint32_t col) {
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_set_size(o, rad * 2, rad * 2);
    lv_obj_align(o, LV_ALIGN_TOP_LEFT, CX - rad, CY - rad);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(col), 0);
    return o;
}
static lv_obj_t *dirLabel(lv_obj_t *p, const char *t, int x, int y) {
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x64748b), 0);
    lv_label_set_text(l, t);
    lv_obj_align(l, LV_ALIGN_TOP_LEFT, x, y);
    return l;
}

void SunApp_Build(lv_obj_t *root) {
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(root, lv_color_hex(0x05070a), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(root);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xf59e0b), 0);
    lv_label_set_text(title, "SOL / INSOLACAO");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    horizon = ringObj(root, RMAX, 0x334155);          // horizonte
    ring45  = ringObj(root, RMAX / 2, 0x1f2937);      // altura 45 graus

    dirLabel(root, "N", CX - 5,        CY - RMAX - 17);
    dirLabel(root, "S", CX - 4,        CY + RMAX + 2);
    dirLabel(root, "L", CX + RMAX + 4, CY - 9);
    dirLabel(root, "O", CX - RMAX - 16,CY - 9);

    facLine = lv_line_create(root);                   // direcao da fachada
    lv_obj_set_style_line_color(facLine, lv_color_hex(0xf59e0b), 0);
    lv_obj_set_style_line_width(facLine, 2, 0);
    lv_obj_set_style_line_dash_width(facLine, 4, 0);
    lv_obj_set_style_line_dash_gap(facLine, 4, 0);

    pathLine = lv_line_create(root);                  // caminho do sol
    lv_obj_set_style_line_color(pathLine, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_line_width(pathLine, 3, 0);
    lv_obj_set_style_line_rounded(pathLine, true, 0);

    sunDot = lv_obj_create(root);                     // sol agora
    lv_obj_set_size(sunDot, 18, 18);
    lv_obj_clear_flag(sunDot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(sunDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sunDot, lv_color_hex(0xfacc15), 0);
    lv_obj_set_style_border_width(sunDot, 2, 0);
    lv_obj_set_style_border_color(sunDot, lv_color_hex(0x713f12), 0);

    lblNow = lv_label_create(root);
    lv_obj_set_style_text_font(lblNow, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblNow, lv_color_hex(0xfacc15), 0);
    lv_obj_align(lblNow, LV_ALIGN_TOP_MID, 0, 252);
    lv_label_set_text(lblNow, "");

    lblA = lv_label_create(root);
    lv_obj_set_style_text_font(lblA, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblA, lv_color_hex(0xe5e7eb), 0);
    lv_obj_align(lblA, LV_ALIGN_TOP_MID, 0, 274);
    lv_label_set_text(lblA, "");

    lblB = lv_label_create(root);
    lv_obj_set_style_text_font(lblB, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblB, lv_color_hex(0x9aa7b4), 0);
    lv_obj_align(lblB, LV_ALIGN_TOP_MID, 0, 294);
    lv_label_set_text(lblB, "");

    lv_obj_t *b = lv_btn_create(root);
    lv_obj_set_size(b, 100, 42);
    lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 322);
    lv_obj_set_style_radius(b, 21, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x374151), 0);
    lv_obj_add_event_cb(b, onBack, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(b);
    lv_label_set_text(bl, "MENU");
    lv_obj_center(bl);

    throttle = 999;   // forca um update no primeiro frame
}

void SunApp_Update() {
    if (++throttle < 40) return;   // ~0,6 s
    throttle = 0;

    int Y = datetime.year, M = datetime.month, D = datetime.day;
    if (Y < 2020 || Y > 2100) { Y = 2026; M = 6; D = 16; }   // RTC sem hora certa
    float curHour = datetime.hour + datetime.minute / 60.0f;

    int n = Sun_Path(Y, M, D, pathAz, pathEl, 48);
    for (int i = 0; i < n; i++) {
        int x, y; mapPolar(pathAz[i], pathEl[i], &x, &y);
        pathPts[i].x = x; pathPts[i].y = y;
    }
    if (n >= 2) { lv_line_set_points(pathLine, pathPts, n);
                  lv_obj_clear_flag(pathLine, LV_OBJ_FLAG_HIDDEN); }
    else        { lv_obj_add_flag(pathLine, LV_OBJ_FLAG_HIDDEN); }

    int fx, fy; mapPolar(sunP.facadeAz, 0, &fx, &fy);
    facPts[0].x = CX; facPts[0].y = CY; facPts[1].x = fx; facPts[1].y = fy;
    lv_line_set_points(facLine, facPts, 2);

    SunResult R; Sun_Compute(Y, M, D, curHour, &R);

    if (R.curUp) {
        int x, y; mapPolar(R.curAz, R.curEl, &x, &y);
        lv_obj_clear_flag(sunDot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(sunDot, LV_ALIGN_TOP_LEFT, x - 9, y - 9);
        char s[40]; snprintf(s, sizeof(s), "sol agora: %.0f graus", (double)R.curEl);
        lv_label_set_text(lblNow, s);
    } else {
        lv_obj_add_flag(sunDot, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lblNow, "sol abaixo do horizonte");
    }

    char r1[12], r2[12], a[48], b[48];
    hm(R.rise, r1, sizeof(r1)); hm(R.set, r2, sizeof(r2));
    snprintf(a, sizeof(a), "nascer %s   por %s", r1, r2);
    if (R.shadowNoon >= 0)
        snprintf(b, sizeof(b), "fachada %.1fh   sombra %.1fm", (double)R.facHours, (double)R.shadowNoon);
    else
        snprintf(b, sizeof(b), "fachada %.1fh de sol direto", (double)R.facHours);
    lv_label_set_text(lblA, a);
    lv_label_set_text(lblB, b);
}
