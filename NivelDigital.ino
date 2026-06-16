// ============================================================================
//  Medidor de obra 4-em-1 (nivel / prumo / declividade / transferidor)
//  Placa: Waveshare ESP32-S3-Touch-LCD-1.46B (tela redonda 412x412 SPD2010 +
//  touch SPD2010 + IMU QMI8658). UI em LVGL 8.
//
//  Drivers da Waveshare reaproveitados (src/): Display/Touch SPD2010, I2C,
//  TCA9554 (expansor), LVGL_Driver, QMI8658, PWR, BAT, RTC.
//  App proprio: LevelApp.* (UI + matematica do acelerometro).
// ============================================================================
#include <Arduino.h>
#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Display_SPD2010.h"
#include "Gyro_QMI8658.h"
#include "RTC_PCF85063.h"
#include "PWR_Key.h"
#include "BAT_Driver.h"
#include "LVGL_Driver.h"
#include "LevelApp.h"
#include "AppUi.h"
#include "SunApp.h"
#include "Mic_dB.h"
#include "WebPortal.h"

// Task de segundo plano (core 0): so atualiza sensores. NAO toca na LVGL
// (LVGL nao e thread-safe; a UI roda toda no loop()/core 1).
static void Sensor_Task(void *p) {
    (void)p;
    while (1) {
        PWR_Loop();
        QMI8658_Loop();           // atualiza Accel.x/.y/.z (e Gyro)
        PCF85063_Loop();
        BAT_Get_Volts();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup() {
    Serial.begin(115200);

    // Bring-up da placa (mesma ordem do demo oficial Waveshare)
    PWR_Init();                   // trava de alimentacao (botao PWR)
    BAT_Init();
    I2C_Init();
    TCA9554PWR_Init(0x00);        // expansor I2C (reset/backlight passam por ele)
    Backlight_Init();
    Set_Backlight(60);            // 0..100
    PCF85063_Init();              // RTC
    QMI8658_Init();               // IMU

    // Tela + LVGL
    LCD_Init();
    Lvgl_Init();

    // UI: splash -> menu -> ferramentas
    AppUi_Init();

    // Decibelimetro (mic I2S) - sobe a propria task no core 0
    MicDB_Init();

    // Portal WiFi (AP + pagina pra ver/baixar as medicoes no celular)
    WebPortal_Init();

    // Sensores num task separado (core 0)
    xTaskCreatePinnedToCore(Sensor_Task, "sensor", 4096, NULL, 3, NULL, 0);
}

void loop() {
    AppUi_Loop();                              // transicao do splash
    if (AppUi_ToolActive())                    // so atualiza a leitura na ferramenta aberta
        LevelApp_Update(Accel.x, Accel.y, Accel.z);
    if (AppUi_SunActive())                     // atualiza a carta solar
        SunApp_Update();
    WebPortal_Loop();                          // atende o celular (servidor HTTP)
    Lvgl_Loop();
    vTaskDelay(pdMS_TO_TICKS(15));
}
