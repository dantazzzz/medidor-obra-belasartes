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

enum { ST_SPLASH, ST_MENU, ST_TOOL, ST_DATA, ST_SUN };
static int      st = ST_SPLASH;
static uint32_t splashStart = 0;

static lv_obj_t *splashCont, *menuCont, *toolCont, *dataCont, *sunCont;

#define NCARDS 7
// Cartoes do menu (5 ferramentas + SOL + DADOS/WiFi)
static const char *CARD_NAME[NCARDS] =
    {"NIVEL", "PRUMO", "DECLIVIDADE", "TRANSFERIDOR", "RUIDO", "SOL", "DADOS"};
static const char *CARD_SUB[NCARDS] =
    {"bolha 2D", "verticalidade", "caimento %", "angulo", "decibelimetro", "carta solar", "WiFi + celular"};
static const uint32_t CARD_COL[NCARDS] =
    {0x2563eb, 0x7c3aed, 0x0891b2, 0xca8a04, 0xdc2626, 0xb45309, 0x0f766e};

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
}

static void onCard(lv_event_t *e) {
    lv_obj_t *card = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(card);
    if      (idx == 5) AppUi_ShowSun();
    else if (idx == 6) AppUi_ShowData();
    else               AppUi_OpenTool(idx);
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

    lv_obj_t *title = lv_label_create(menuCont);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xe2e8f0), 0);
    lv_label_set_text(title, "MEDIDOR DE OBRA");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    // carrossel: linha de cartoes com snap no centro
    lv_obj_t *car = lv_obj_create(menuCont);
    lv_obj_set_size(car, 412, 280);
    lv_obj_align(car, LV_ALIGN_CENTER, 0, 6);
    lv_obj_set_style_bg_opa(car, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(car, 0, 0);
    lv_obj_set_flex_flow(car, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(car, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_snap_x(car, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_dir(car, LV_DIR_HOR);
    lv_obj_add_flag(car, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scrollbar_mode(car, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_left(car, 66, 0);
    lv_obj_set_style_pad_right(car, 66, 0);
    lv_obj_set_style_pad_column(car, 18, 0);

    for (int i = 0; i < NCARDS; i++) {
        lv_obj_t *card = lv_obj_create(car);
        lv_obj_set_size(card, 280, 250);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(card, 28, 0);
        lv_obj_set_style_bg_color(card, lv_color_hex(CARD_COL[i]), 0);
        lv_obj_set_style_border_width(card, 0, 0);
        lv_obj_set_style_shadow_width(card, 0, 0);
        lv_obj_set_user_data(card, (void *)(intptr_t)i);
        lv_obj_add_event_cb(card, onCard, LV_EVENT_CLICKED, NULL);

        lv_obj_t *n = lv_label_create(card);
        lv_obj_set_style_text_font(n, &lv_font_montserrat_22, 0);
        lv_obj_set_style_text_color(n, lv_color_hex(0xffffff), 0);
        lv_label_set_text(n, CARD_NAME[i]);
        lv_obj_align(n, LV_ALIGN_CENTER, 0, -16);

        lv_obj_t *s = lv_label_create(card);
        lv_obj_set_style_text_font(s, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(s, lv_color_hex(0xe5e7eb), 0);
        lv_label_set_text(s, CARD_SUB[i]);
        lv_obj_align(s, LV_ALIGN_CENTER, 0, 20);

        lv_obj_t *go = lv_label_create(card);
        lv_obj_set_style_text_font(go, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(go, lv_color_hex(0xffffff), 0);
        lv_label_set_text(go, "toque para abrir");
        lv_obj_align(go, LV_ALIGN_BOTTOM_MID, 0, -16);
    }

    lv_obj_t *hint = lv_label_create(menuCont);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x64748b), 0);
    lv_label_set_text(hint, "<  arraste para o lado  >");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -26);
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

void AppUi_ShowMenu()   { show(ST_MENU); }
void AppUi_ShowData()   { show(ST_DATA); }
void AppUi_ShowSun()    { show(ST_SUN); }
bool AppUi_ToolActive() { return st == ST_TOOL; }
bool AppUi_SunActive()  { return st == ST_SUN; }
