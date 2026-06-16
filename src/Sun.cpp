// Astronomia solar offline (ver Sun.h). Padrao: Sao Paulo, fachada Norte, h=3m.
#include "Sun.h"
#include <math.h>

SunParams sunP = { -23.55f, -46.63f, -3.0f, 0.0f, 3.0f };

static const float RAD  = 0.01745329252f;
static const float DEGc = 57.29577951f;

static int dayOfYear(int y, int m, int d) {
    static const int cum[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
    if (m < 1) m = 1; if (m > 12) m = 12;
    int n = cum[m-1] + d;
    if (m > 2 && ((y%4==0 && y%100!=0) || y%400==0)) n += 1;
    return n;
}
static float declN(int N) { return 23.45f * sinf(RAD*360.0f*(284+N)/365.0f); }
static float eotN(int N)  { float B = RAD*360.0f*(N-81)/364.0f;
    return 9.87f*sinf(2*B) - 7.53f*cosf(B) - 1.5f*sinf(B); }

static void sunPos(int N, float t, float *el, float *az) {
    float dec = declN(N)*RAD, E = eotN(N);
    float TC  = 4*(sunP.lon - 15*sunP.tz) + E;
    float LST = t + TC/60.0f;
    float H   = (15*(LST-12))*RAD, phi = sunP.lat*RAD;
    float se  = sinf(phi)*sinf(dec) + cosf(phi)*cosf(dec)*cosf(H);
    if (se > 1) se = 1; if (se < -1) se = -1;
    float e = asinf(se);
    *el = e*DEGc;
    float cA = (sinf(dec) - sinf(phi)*sinf(e)) / (cosf(phi)*cosf(e));
    if (cA > 1) cA = 1; if (cA < -1) cA = -1;
    float A = acosf(cA)*DEGc;
    if (sinf(H) > 0) A = 360 - A;
    *az = A;
}

void Sun_Compute(int y, int m, int d, float curHour, SunResult *o) {
    int N = dayOfYear(y, m, d);
    float dec = declN(N)*RAD, phi = sunP.lat*RAD, E = eotN(N);
    float TC = 4*(sunP.lon - 15*sunP.tz) + E;
    float noon = 12 - TC/60.0f;
    float c = -tanf(phi)*tanf(dec);
    o->valid = (c <= 1 && c >= -1);
    float H0 = o->valid ? acosf(c)*DEGc/15.0f : 0;
    o->rise = noon - H0; o->set = noon + H0; o->noon = noon; o->dayLen = 2*H0;

    float el, az;
    sunPos(N, noon, &el, &az);
    o->maxEl = el;
    o->shadowNoon = (el > 1) ? (sunP.objH / tanf(el*RAD)) : -1;

    sunPos(N, curHour, &o->curEl, &o->curAz);
    o->curUp = o->curEl > 0;

    o->facHours = 0; o->facStart = -1; o->facEnd = -1;
    if (o->valid) {
        for (float t = o->rise; t <= o->set; t += 1.0f/60.0f) {
            float e2, a2; sunPos(N, t, &e2, &a2);
            if (e2 > 0 && cosf((a2 - sunP.facadeAz)*RAD) > 0) {
                o->facHours += 1.0f/60.0f;
                if (o->facStart < 0) o->facStart = t;
                o->facEnd = t;
            }
        }
    }
}

int Sun_Path(int y, int m, int d, float *az, float *el, int maxN) {
    int N = dayOfYear(y, m, d);
    float dec = declN(N)*RAD, phi = sunP.lat*RAD, E = eotN(N);
    float TC = 4*(sunP.lon - 15*sunP.tz) + E, noon = 12 - TC/60.0f;
    float c = -tanf(phi)*tanf(dec);
    if (c > 1 || c < -1) return 0;
    float H0 = acosf(c)*DEGc/15.0f, rise = noon - H0, set = noon + H0;
    if (maxN < 2) return 0;
    float step = (set - rise) / (maxN - 1);
    for (int i = 0; i < maxN; i++) {
        float e, a; sunPos(N, rise + i*step, &e, &a);
        if (e < 0) e = 0;
        az[i] = a; el[i] = e;
    }
    return maxN;
}
