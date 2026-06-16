// ============================================================================
//  LevelApp.h  -  tela de uma ferramenta de medicao (nivel/prumo/declividade/
//  transferidor/ruido). UI em LVGL 8 + acelerometro (QMI8658) / mic (I2S).
// ============================================================================
#pragma once
#include <lvgl.h>

// Monta a tela do medidor DENTRO de um container (parent). Chamar 1x.
void LevelApp_Build(lv_obj_t *parent);

// Atualiza a leitura na tela (chamar no loop quando a ferramenta esta ativa).
void LevelApp_Update(float ax, float ay, float az);

// Define o modo ativo: 0 NIVEL 1 PRUMO 2 DECLIVIDADE 3 TRANSFERIDOR 4 RUIDO
void LevelApp_SetMode(int mode);

// Leitura atual (para a pagina WiFi mostrar ao vivo)
const char *LevelApp_ModeName();
float       LevelApp_Value();
const char *LevelApp_Unit();

// Ajustes (tela de configuracoes)
void LevelApp_FlipX();
void LevelApp_FlipY();
bool LevelApp_SignXPos();
bool LevelApp_SignYPos();
void LevelApp_SetBeep(bool on);
bool LevelApp_GetBeep();
void LevelApp_Calibrate();   // captura a leitura atual como zero (superficie plana)
