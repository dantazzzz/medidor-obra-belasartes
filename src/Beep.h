#pragma once
// Bip pela saida de audio I2S (PCM5101) da placa. Precisa de um alto-falante
// ligado na saida de audio. Usado no "assistente sonoro de nivelamento".
void Beep_Init();
void Beep_Beep(int freq);   // pede um bip curto na frequencia (Hz); nao bloqueia
bool Beep_Available();      // true se o I2S de saida iniciou
