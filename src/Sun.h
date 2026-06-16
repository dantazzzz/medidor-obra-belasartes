#pragma once
// Astronomia solar (offline) — posicao do sol, nascer/por, insolacao de fachada,
// sombra. Os mesmos calculos da pagina web, agora em C para desenhar na tela.

struct SunParams { float lat, lon, tz, facadeAz, objH; };
extern SunParams sunP;   // local/orientacao atuais (o celular pode mudar via /setsol)

struct SunResult {
    bool  valid;             // false = sol nao nasce/poe (regiao polar)
    float rise, set, noon;   // horas (0..24)
    float dayLen, maxEl;
    float facHours, facStart, facEnd;   // sol direto na fachada
    float shadowNoon;        // sombra ao meio-dia (m); <0 = sem
    float curEl, curAz;      // posicao do sol agora
    bool  curUp;             // sol acima do horizonte agora
};

void Sun_Compute(int y, int m, int d, float curHour, SunResult *out);
int  Sun_Path(int y, int m, int d, float *az, float *el, int maxN); // pontos do caminho
