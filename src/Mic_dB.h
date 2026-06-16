#pragma once
// Decibelimetro leve: le o microfone I2S (MEMS) da placa e calcula o nivel
// sonoro aproximado em dB. Sem reconhecimento de voz (ESP_SR) - so RMS.
void  MicDB_Init();   // inicia o I2S do mic + task de leitura (core 0)
float MicDB_Get();    // ultimo nivel em dB (aproximado, calibravel)
