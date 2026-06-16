// ============================================================================
//  Mic_dB.cpp  -  decibelimetro aproximado usando o mic I2S (MEMS) da placa
// ----------------------------------------------------------------------------
//  Pinos I2S do mic (do demo oficial Waveshare 1.46B): BCK=15, WS=2, DIN=39.
//  Le blocos do I2S, tira o nivel DC, calcula RMS e converte pra dB.
//
//  CALIBRACAO: o numero e aproximado. Compare com um app de decibelimetro do
//  celular e ajuste SPL_REF (cada +1 aqui sobe ~1 dB na leitura).
// ============================================================================
#include "Mic_dB.h"
#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

#define MIC_BCK   15
#define MIC_WS    2
#define MIC_DOUT  -1
#define MIC_DIN   39
#define MIC_RATE  16000
#define SPL_REF   120.0f   // 0 dBFS ~ 120 dB SPL (mic MEMS tipico). Ajuste fino aqui.

static I2SClass     s_i2s;
static volatile float s_db = 40.0f;

static void MicTask(void *p) {
    (void)p;
    static int16_t buf[512];           // 512 amostras int16 (L/R interleaved)
    for (;;) {
        size_t n  = s_i2s.readBytes((char *)buf, sizeof(buf));
        int    cnt = (int)(n / sizeof(int16_t));
        if (cnt >= 16) {
            double sum = 0;
            for (int i = 0; i < cnt; i++) sum += buf[i];
            double mean = sum / cnt;            // nivel DC
            double acc = 0;
            for (int i = 0; i < cnt; i++) {
                double s = (double)buf[i] - mean;
                acc += s * s;
            }
            double rms = sqrt(acc / cnt);
            float  db  = (rms < 1.0) ? 0.0f
                         : 20.0f * log10f((float)(rms / 32768.0)) + SPL_REF;
            if (db < 0) db = 0;
            s_db += 0.3f * (db - s_db);          // suaviza
        } else {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

void MicDB_Init() {
    s_i2s.setPins(MIC_BCK, MIC_WS, MIC_DOUT, MIC_DIN);
    s_i2s.setTimeout(200);
    s_i2s.begin(I2S_MODE_STD, MIC_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    xTaskCreatePinnedToCore(MicTask, "micdb", 4096, NULL, 4, NULL, 0);
}

float MicDB_Get() { return s_db; }
