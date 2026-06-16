#pragma once
#include <stdint.h>
// Registro (em RAM) das medicoes capturadas. A pagina web (WiFi) le isto.
#define DATALOG_MAX 60

struct LogEntry {
    char  when[12];   // "hh:mm:ss"
    char  mode[16];   // ex: "DECLIVIDADE"
    float value;
    char  unit[6];    // ex: "%", "dB", "GRAUS"
};

void            DataLog_Add(const char *mode, float value, const char *unit);
int             DataLog_Count();
const LogEntry *DataLog_Get(int i);
void            DataLog_Clear();
