#pragma once
#include <lvgl.h>
// Ferramenta SOL: desenha a carta solar (cupula do ceu) na tela redonda, com o
// caminho do sol no dia, a posicao do sol agora (RTC), a fachada e os numeros.
void SunApp_Build(lv_obj_t *parent);
void SunApp_Update();   // recomputa+redesenha (no loop, quando a tela SOL esta ativa)
