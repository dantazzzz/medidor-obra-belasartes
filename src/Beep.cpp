// Bip via I2S de saida (PCM5101). Roda numa task no core 0 pra nao travar a UI.
// Pinos do audio (demo Waveshare 1.46B): BCLK=48, WS/LRC=38, DOUT=47.
#include "Beep.h"
#include <Arduino.h>
#include <ESP_I2S.h>
#include <math.h>

#define BEEP_BCLK 48
#define BEEP_WS   38
#define BEEP_DOUT 47
#define BEEP_RATE 16000

static I2SClass       s_out;
static volatile int   s_req = 0;     // frequencia pedida (0 = nada)
static bool           s_ok  = false;

static void BeepTask(void *p) {
    (void)p;
    static int16_t buf[1600];        // ate 800 frames estereo
    for (;;) {
        int f = s_req;
        if (f > 0 && s_ok) {
            s_req = 0;
            int frames = BEEP_RATE * 45 / 1000;       // ~45 ms
            if (frames > 800) frames = 800;
            for (int i = 0; i < frames; i++) {
                float t = (float)i / BEEP_RATE;
                int16_t s = (int16_t)(6000.0f * sinf(2.0f * 3.14159265f * f * t));
                buf[2 * i] = s; buf[2 * i + 1] = s;
            }
            s_out.write((uint8_t *)buf, frames * 2 * sizeof(int16_t));
        } else {
            vTaskDelay(pdMS_TO_TICKS(8));
        }
    }
}

void Beep_Init() {
    s_out.setPins(BEEP_BCLK, BEEP_WS, BEEP_DOUT, -1);
    s_ok = s_out.begin(I2S_MODE_STD, BEEP_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    xTaskCreatePinnedToCore(BeepTask, "beep", 4096, NULL, 4, NULL, 0);
}
void Beep_Beep(int freq) { if (s_ok && s_req == 0) s_req = freq; }
bool Beep_Available()    { return s_ok; }
