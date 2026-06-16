#pragma once
// Orquestra as telas: SPLASH (logo) -> MENU (swipe) -> FERRAMENTA -> MENU...
void AppUi_Init();          // monta tudo e mostra o splash
void AppUi_Loop();          // no loop: cuida do timeout do splash
void AppUi_OpenTool(int mode);
void AppUi_ShowMenu();
void AppUi_ShowData();      // tela com instrucoes de WiFi
void AppUi_ShowSun();       // ferramenta carta solar
bool AppUi_ToolActive();    // true quando uma ferramenta de medicao esta aberta
bool AppUi_SunActive();     // true quando a carta solar esta aberta
