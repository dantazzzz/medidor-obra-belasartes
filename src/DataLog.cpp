// Registro das medicoes: em RAM (pagina WiFi) e tambem no cartao SD (permanente).
#include "DataLog.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "FS.h"
#include "SD_MMC.h"
#include <string.h>
#include <stdio.h>

static LogEntry s_log[DATALOG_MAX];
static int      s_count = 0;

void DataLog_Add(const char *mode, float value, const char *unit) {
    LogEntry e;
    snprintf(e.when, sizeof(e.when), "%02d:%02d:%02d",
             datetime.hour, datetime.minute, datetime.second);
    strncpy(e.mode, mode, sizeof(e.mode) - 1); e.mode[sizeof(e.mode) - 1] = 0;
    strncpy(e.unit, unit, sizeof(e.unit) - 1); e.unit[sizeof(e.unit) - 1] = 0;
    e.value = value;

    if (s_count < DATALOG_MAX) {
        s_log[s_count++] = e;
    } else {                                    // cheio: descarta o mais antigo
        memmove(s_log, s_log + 1, sizeof(LogEntry) * (DATALOG_MAX - 1));
        s_log[DATALOG_MAX - 1] = e;
    }

    // grava tambem no cartao SD (permanente), se houver cartao montado
    if (SDCard_Size > 0) {
        bool novo = !SD_MMC.exists("/medicoes.csv");
        File f = SD_MMC.open("/medicoes.csv", FILE_APPEND);
        if (f) {
            if (novo) f.println("data,hora,funcao,valor,unidade");
            f.printf("%04d-%02d-%02d,%s,%s,%.1f,%s\n",
                     datetime.year, datetime.month, datetime.day,
                     e.when, e.mode, e.value, e.unit);
            f.close();
        }
    }
}

int             DataLog_Count()        { return s_count; }
const LogEntry *DataLog_Get(int i)     { return (i >= 0 && i < s_count) ? &s_log[i] : nullptr; }
void            DataLog_Clear()        { s_count = 0; }
