#pragma once
// Portal WiFi: a placa cria uma rede (AP) e serve uma pagina HTML pra ver/baixar
// as medicoes salvas pelo celular. Tambem deixa acertar o relogio (hora do cel).
#define AP_SSID "Medidor-Obra"
#define AP_PASS "belasartes"      // >= 8 caracteres (WPA2)
#define AP_URL  "192.168.4.1"

void WebPortal_Init();
void WebPortal_Loop();
