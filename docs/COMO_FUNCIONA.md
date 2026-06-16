# 🔧 Como Funciona — Documento Técnico

**Medidor de Obra — Belas Artes (FEBASP)**
Placa: Waveshare ESP32‑S3‑Touch‑LCD‑1.46B (tela redonda 412×412, toque, IMU, microfone, RTC, WiFi).

Este documento explica **como foi feito o cálculo de cada função**, com as fórmulas reais usadas no firmware. Para cada item há: a **ideia física**, a **fórmula** (em Markdown), a **origem** e **como o código faz** (com o arquivo citado).

> Todos os números e fórmulas aqui foram lidos diretamente dos arquivos `.cpp` reais. Nada foi inventado.

---

## 📐 1. Os ângulos de inclinação (base do nível, prumo, declividade e transferidor)

**Ideia física.** Em repouso, o acelerômetro mede só a gravidade. A direção do vetor gravidade dentro da placa diz o quanto ela está inclinada. Decompondo a gravidade nos três eixos (`ax`, `ay`, `az`) dá para recuperar os dois ângulos de inclinação: **roll** (giro lateral) e **pitch** (caimento para frente/trás).

**Fórmulas.**

```
roll  = atan2(ay, az) · (180/π)
pitch = atan2(−ax, √(ay² + az²)) · (180/π)
```

O fator `180/π ≈ 57.2958` converte radianos em graus (no código é a constante `DEG = 57.29577951`).

**Origem.** Decomposição clássica do vetor gravidade de um acelerômetro de 3 eixos (fórmula padrão de tilt‑sensing). O `atan2` é usado em vez de `atan` para cobrir os 360° corretamente e tratar os sinais dos eixos. O `pitch` usa a raiz `√(ay²+az²)` para projetar a gravidade no plano perpendicular ao eixo X, deixando o pitch independente do roll.

**Como o código faz** — `src/LevelApp.cpp`, função `LevelApp_Update(ax, ay, az)`:

```c
float roll  = atan2f(ay, az) * DEG;
float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * DEG;
```

Os dados brutos `ax, ay, az` vêm do IMU **QMI8658** (lido pelo driver `Gyro_QMI8658`) e são passados para essa função no loop.

### Filtro passa‑baixa (suavização)

**Ideia física.** A leitura crua treme (vibração, ruído, mão tremendo). Um filtro passa‑baixa exponencial (média móvel ponderada) deixa o valor estável sem perder muito a resposta.

**Fórmula.**

```
filtrado ← filtrado + SMOOTH · (novo − filtrado)
```

Com `SMOOTH = 0.12`: a cada amostra o valor caminha 12% na direção da leitura nova. Menor = mais suave/estável; maior = mais rápido/nervoso.

**Como o código faz** — `src/LevelApp.cpp`:

```c
fRoll  += SMOOTH * (roll  - fRoll);
fPitch += SMOOTH * (pitch - fPitch);
```

### Zerar (referência relativa)

Ao tocar **ZERAR**, o ângulo atual vira o "zero" e todas as leituras passam a ser relativas a ele (`r = fRoll − roll0`, `p = fPitch − pitch0`). Serve para calibrar contra uma superfície de referência que não está perfeitamente plana. Tolerância de "nivelado": `LEVEL_TOL = 0.4°`.

---

## 🟢 2. NÍVEL (bolha 2D) — modo 0

**Ideia física.** Um nível de bolha clássico em duas direções ao mesmo tempo. A "distância" da bolha ao centro é o desvio total combinando os dois eixos.

**Fórmula.**

```
desvio_total = √(roll² + pitch²)
```

Posição da bolha na tela (offset em pixels a partir do centro):

```
offset_x = +roll  · PXDEG
offset_y = −pitch · PXDEG     (limitado a ±120 px)
```

Com `PXDEG = 3.0` px/grau. O sinal de Y é invertido (`SIGN_Y = −1`) para a bolha "rolar" no sentido físico esperado.

**Como o código faz** — `src/LevelApp.cpp`, `case 0`:

```c
bx = r; by = p; big = sqrtf(r * r + p * p); levelOk = big < LEVEL_TOL;
```

A bolha fica verde‑clara e mostra **NIVELADO** quando `big < 0.4°`.

---

## 📏 3. PRUMO — modo 1

**Ideia física.** Verificar se algo está vertical (uma parede, um batente). Encostando a placa na superfície vertical, interessa um único eixo (o caimento).

**Fórmula.**

```
prumo = | pitch |
```

**Como o código faz** — `src/LevelApp.cpp`, `case 1`:

```c
bx = 0; by = p; big = fabsf(p); levelOk = fabsf(p) < LEVEL_TOL;
```

A bolha só se move na vertical (`bx = 0`). Mostra **PRUMO OK** quando `|pitch| < 0.4°`.

---

## 📉 4. DECLIVIDADE (%) — modo 2

**Ideia física.** Em obra, caimento se mede em **porcentagem**: quantos centímetros sobe/desce a cada 100 cm na horizontal. Isso é exatamente a tangente do ângulo de inclinação.

**Fórmula.**

```
declividade(%) = tan(pitch) · 100
```

(o `pitch` é convertido de volta para radianos antes da tangente: `tan(pitch/DEG)`).

**Origem.** Definição de inclinação em porcentagem: `100 · (subida / distância horizontal) = 100 · tan(ângulo)`.

**Como o código faz** — `src/LevelApp.cpp`, `case 2`:

```c
bx = 0; by = p; big = tanf(p / DEG) * 100.0f; unit = "%";
```

### Verificação contra normas

A tela compara o valor com presets de norma (toque no texto azul para alternar). Os limites estão na tabela `NORMS[]`:

| Preset | Mín. | Máx. |
|---|---|---|
| RAMPA NBR9050 | — | 8,33 % |
| PISO p/ RALO | 0,5 % | 2 % |
| ESGOTO 100 mm | 1 % | — |
| ÁGUA PLUVIAL | 0,5 % | — |
| LAJE IMPERM. | 1 % | — |
| LIVRE | — | — |

A lógica de aprovação (mostra **DENTRO DA NORMA** ou **FORA**):

```c
bool ok = (nm.mn < 0 || v >= nm.mn) && (nm.mx < 0 || v <= nm.mx);
```

onde `v = |declividade|` e `−1` significa "sem limite". **Estes valores são um guia — confira sempre a norma vigente** (a RAMPA NBR 9050 tem faixas e regras de patamar que não estão todas embutidas aqui).

---

## 📐 5. TRANSFERIDOR — modo 3

**Ideia física.** Medir um ângulo qualquer combinando as duas inclinações, sem checagem de "nivelado".

**Fórmula.**

```
ângulo = √(roll² + pitch²)
```

**Como o código faz** — `src/LevelApp.cpp`, `case 3`:

```c
bx = r; by = p; big = sqrtf(r * r + p * p);
```

É o mesmo cálculo do nível 2D (item 2), mas exibido como ângulo em graus, com a bolha livre nos dois eixos.

---

## 🔊 6. RUÍDO / Decibelímetro (dB) — modo 4

**Ideia física.** Som é variação de pressão no ar. O microfone MEMS entrega amostras digitais dessa variação. A "intensidade" do som é o valor **RMS** (raiz da média dos quadrados) do sinal, e o nível em decibéis é o logaritmo dessa intensidade.

**Fórmulas.**

1. **Remover o nível DC** (offset constante do microfone):

```
DC = média(amostras)
s[i] = amostra[i] − DC
```

2. **RMS** (energia do sinal sem o DC):

```
RMS = √( Σ s[i]² / N )
```

3. **Converter para decibéis** (referência de fundo de escala):

```
dB = 20 · log10( RMS / 32768 ) + SPL_REF
```

Com `SPL_REF = 120.0`: assume que 0 dBFS (fundo de escala do conversor de 16 bits, valor 32768) corresponde a ~120 dB SPL — típico de microfone MEMS. **Este é o número de calibração**; cada +1 em `SPL_REF` sobe ~1 dB na leitura. Para calibrar, compare com um app de decibelímetro do celular e ajuste.

**Origem.** Definição de nível em dB: `dB = 20·log10(amplitude/referência)`. O `20·log10` (e não `10·log10`) é usado porque RMS é uma **amplitude** (pressão), não uma potência.

**Como o código faz** — `src/Mic_dB.cpp`, função `MicTask()`:

```c
double mean = sum / cnt;            // nivel DC
double acc = 0;
for (int i = 0; i < cnt; i++) { double s = (double)buf[i] - mean; acc += s * s; }
double rms = sqrt(acc / cnt);
float db = (rms < 1.0) ? 0.0f
         : 20.0f * log10f((float)(rms / 32768.0)) + SPL_REF;
```

A leitura ainda passa por um filtro de suavização próprio (`s_db += 0.3f * (db − s_db)`) e roda numa **task FreeRTOS** separada (`xTaskCreatePinnedToCore(..., core 0)`) lendo blocos do I2S a 16 kHz. O microfone é configurado em `MicDB_Init()` com pinos **BCK=15, WS=2, DIN=39**.

### Faixas de alerta na tela

| Valor | Cor | Texto |
|---|---|---|
| < 70 dB | verde | OK |
| 70–85 dB | amarelo | ATENÇÃO |
| ≥ 85 dB | vermelho | ALTO (NR‑15) |

(O limite de 85 dB remete ao limite de exposição ocupacional da NR‑15. A medição é **aproximada** e sem ponderação A — não substitui um dosímetro calibrado.)

---

## 🔁 7. CONVERSOR — modo 5

**Ideia física.** A mesma inclinação pode ser expressa de várias formas — graus, porcentagem, proporção e mm por metro. Esta ferramenta mostra todas ao mesmo tempo, partindo do ângulo medido pela bolha (o mesmo cálculo do nível 2D/transferidor).

**Fórmulas.** Partindo do ângulo `big = √(roll² + pitch²)` (em graus):

```
porcentagem (%)   = tan(big) · 100
mm por metro      = tan(big) · 1000
proporção (1:X)   = 1 / tan(big)      (só se big > 0.2°; senão "plano")
```

(o `big` é convertido para radianos antes da tangente: `tan(big/DEG)`).

**Origem.** São identidades da mesma inclinação. A porcentagem é a tangente vezes 100; `mm/m` é a subida em milímetros para 1000 mm de horizontal (tangente vezes 1000); a proporção 1:X é o inverso da tangente — para cada 1 de subida, X de horizontal.

**Como o código faz** — `src/LevelApp.cpp`, `case 5` e bloco `g_mode == 5`:

```c
snprintf(s1, ..., "%.1f%%    %.0f mm/m",
         tanf(big / DEG) * 100.0f, tanf(big / DEG) * 1000.0f);
if (big > 0.2f) snprintf(s2, ..., "proporcao  1:%.0f", 1.0f / tanf(big / DEG));
else            snprintf(s2, ..., "plano");
```

---

## 📐 8. ESQUADRO — modo 6

**Ideia física.** Conferir se uma quina/canto forma 90° (esquadro). A placa mede o ângulo total `big = √(roll² + pitch²)` e a ferramenta compara com 90°.

**Fórmula.**

```
desvio = big − 90°
esquadro OK  ⟺  |desvio| < 1°
```

Fica **verde** (com o texto "ESQUADRO OK (90)" / "quina reta") quando dentro de ±1° de 90; fora disso mostra o desvio assinado (ex.: `+2.3 de 90 graus`).

**Como o código faz** — `src/LevelApp.cpp`, bloco `g_mode == 6`:

```c
float d = big - 90.0f;
bool ok = fabsf(d) < 1.0f;
... snprintf(s, ..., "%+.1f de 90 graus", d);
```

---

## 📊 9. PLANEZA — modo 7

**Ideia física.** Passar a placa pela superfície (piso, parede, bancada) e ver o quanto ela "balança" — a diferença entre o ponto mais inclinado e o mais nivelado revela ondulações/desnível. Não interessa o valor instantâneo, e sim a **faixa** percorrida.

**Fórmula.** A cada amostra calcula o ângulo total `big = √(roll² + pitch²)` e mantém o mínimo e o máximo vistos desde o último ZERAR:

```
planMin ← min(planMin, big)
planMax ← max(planMax, big)
desvio  = planMax − planMin        [graus]
```

ZERAR (ou entrar no modo) reinicia a faixa: `planMin = 999`, `planMax = 0`.

**Como o código faz** — `src/LevelApp.cpp`, `case 7` e bloco `g_mode == 7`:

```c
case 7: bx = r; by = p; big = sqrtf(r * r + p * p);
        if (big < planMin) planMin = big;
        if (big > planMax) planMax = big; break;
...
snprintf(s, ..., "desvio: %.1f graus", planMax - planMin);
snprintf(s, ..., "min %.1f   max %.1f", planMin, planMax);
```

---

## 📈 10. PERFIL — modo 8

**Ideia física.** Levantar o caimento de uma superfície ao longo de um trajeto (ex.: perfil de uma calçada ou laje), em **porcentagem**, registrando o menor, o maior e a média. Como na DECLIVIDADE, o valor é a tangente do pitch.

**Fórmulas.**

```
caimento (%) = tan(pitch) · 100
perfMin ← min(perfMin, caimento)
perfMax ← max(perfMax, caimento)
média   ≈ (perfMin + perfMax) / 2     [%]
```

(o `pitch` é convertido para radianos antes da tangente). A média é a aproximação ponto‑médio da faixa registrada — útil como caimento representativo do trecho. ZERAR reinicia: `perfMin = 999`, `perfMax = −999`.

**Como o código faz** — `src/LevelApp.cpp`, `case 8` e bloco `g_mode == 8`:

```c
case 8: bx = 0; by = p; big = tanf(p / DEG) * 100.0f; unit = "%";
        if (big < perfMin) perfMin = big;
        if (big > perfMax) perfMax = big; break;
...
float md = (perfMax > -998) ? (perfMin + perfMax) / 2.0f : big;
snprintf(s, ..., "media ~%.1f%%", md);
snprintf(s, ..., "min %.1f%%   max %.1f%%", perfMin, perfMax);
```

---

## 🎯 11. CALIBRAÇÃO do nível (offset persistente)

**Ideia física.** Nenhuma placa fica perfeitamente alinhada com o corpo do aparelho — sempre há um pequeno desvio de montagem do IMU. A calibração (botão **CAL** em Ajustes) mede esse desvio uma vez, apoiando o aparelho numa superfície sabidamente plana, e desconta esse offset de todas as leituras seguintes.

**Fórmulas.** Ao calibrar, guarda‑se o ângulo filtrado atual como offset de fábrica:

```
gCalR = fRoll        (offset de roll)
gCalP = fPitch       (offset de pitch)
```

Depois, toda leitura aplica **dois** descontos — o offset de calibração `gCal*` e o zero relativo `*0` (do botão ZERAR):

```
r = (fRoll  − gCalR) − roll0
p = (fPitch − gCalP) − pitch0
```

**Diferença entre CAL e ZERAR.** O `gCal*` é o "zero de fábrica" (desvio fixo do sensor); o `roll0/pitch0` é um zero temporário contra a peça que se está medindo. Quando se aperta ZERAR, o código já considera a calibração: `roll0 = fRoll − gCalR`.

**Como o código faz** — `src/LevelApp.cpp`, `LevelApp_Calibrate()` e `LevelApp_Update()`:

```c
void LevelApp_Calibrate() { gCalR = fRoll; gCalP = fPitch; roll0 = 0; pitch0 = 0; }
...
float r = (fRoll - gCalR) - roll0;
float p = (fPitch - gCalP) - pitch0;
```

---

## 🔋 12. Indicador de bateria (%)

**Ideia física.** A bateria de lítio tem tensão que cai conforme descarrega. Medindo essa tensão dá para estimar a carga restante de forma aproximada, mapeando a faixa útil (~3,3 V vazia → ~4,2 V cheia) para 0–100 %.

**Fórmulas.**

1. **Tensão da bateria** — o ADC lê a tensão num divisor; o driver multiplica de volta pelo fator do divisor (×3) e divide pela correção de offset:

```
V = (mV_lidos · 3 / 1000) / Measurement_offset      [volts]
```

2. **Porcentagem** — mapeamento linear da faixa útil, limitado a 0–100 %:

```
% = (V − 3.30) / 0.90 · 100        (clamp 0…100)
```

O `0.90` é a largura da janela usada (3,30 V → 4,20 V). É uma estimativa **linear simples** — a curva real de descarga do lítio não é reta, então serve como indicação grosseira, não como medidor preciso.

**Origem / código.**
- Tensão: `BAT_Get_Volts()` em `src/BAT_Driver.cpp` (ADC de 12 bits em **GPIO8**, `analogReadMilliVolts`).
- Porcentagem: barra de status em `src/LevelApp.cpp`:

```c
int pct = (int)((BAT_analogVolts - 3.30f) / 0.90f * 100.0f);
if (pct < 0) pct = 0; if (pct > 100) pct = 100;
```

A barra de status no topo de cada ferramenta mostra `HH:MM` (hora do RTC) + a bateria em %.

---

## ☀️ 13. Carta solar / Insolação (astronomia solar offline)

Esta é a parte mais matemática. O **mesmo cálculo existe em dois lugares**: em **C** (`src/Sun.cpp`, para desenhar na tela do ESP) e em **JavaScript** (página `/sol` dentro de `src/WebPortal.cpp`, para rodar no celular). As fórmulas são idênticas; abaixo elas aparecem uma vez, com referência aos dois arquivos.

### 13.1 Dia do ano (N)

Número do dia no ano (1 a 365/366), com correção de ano bissexto. É a entrada de quase tudo.

- **C** — `dayOfYear()` em `src/Sun.cpp` (tabela cumulativa de meses + ajuste de bissexto).
- **JS** — `doy()` em `src/WebPortal.cpp` (diferença de datas UTC).

### 13.2 Declinação solar (δ)

**Ideia física.** A inclinação do eixo da Terra (~23,45°) faz o sol "subir e descer" no céu ao longo do ano. A declinação é o ângulo do sol em relação ao equador celeste.

**Fórmula.**

```
δ = 23.45° · sin( 360° · (284 + N) / 365 )
```

**Origem.** Equação de Cooper, aproximação padrão da declinação solar.

**Código.** `declN()` em `src/Sun.cpp` e `decl()` em `src/WebPortal.cpp`:

```c
return 23.45f * sinf(RAD*360.0f*(284+N)/365.0f);
```

### 13.3 Equação do tempo (EoT)

**Ideia física.** O sol "verdadeiro" adianta ou atrasa em relação ao relógio (até ~±15 min) por causa da órbita elíptica e da inclinação do eixo. A EoT corrige isso.

**Fórmula.**

```
B   = 360° · (N − 81) / 364
EoT = 9.87·sin(2B) − 7.53·cos(B) − 1.5·sin(B)   [minutos]
```

**Origem.** Aproximação clássica de Spencer/duffie da equação do tempo.

**Código.** `eotN()` em `src/Sun.cpp` e `eot()` em `src/WebPortal.cpp`.

### 13.4 Correção de tempo e hora solar

**Ideia física.** Converter a hora do relógio (no fuso `tz`) para a **hora solar local** real do ponto (longitude `lon`), combinando o deslocamento de longitude com a EoT.

**Fórmulas.**

```
TC  = 4 · (lon − 15·tz) + EoT        [minutos]   (4 min por grau de longitude)
LST = hora_relógio + TC/60           [hora solar local]
H   = 15° · (LST − 12)               [ângulo horário; 15°/h, 0 ao meio-dia]
```

**Código.** Dentro de `sunPos()` em `src/Sun.cpp` e `pos()` em `src/WebPortal.cpp`.

### 13.5 Elevação (altura) do sol

**Ideia física.** Quão alto o sol está acima do horizonte, dado o ângulo horário `H`, a latitude `φ` e a declinação `δ`.

**Fórmula.**

```
sin(el) = sin(φ)·sin(δ) + cos(φ)·cos(δ)·cos(H)
el = arcsin( sin(el) )
```

**Origem.** Fórmula fundamental do triângulo astronômico (esférico).

**Código.** `sunPos()` em `src/Sun.cpp`:

```c
float se = sinf(phi)*sinf(dec) + cosf(phi)*cosf(dec)*cosf(H);
float e = asinf(se);   *el = e * DEGc;
```

Idêntica em `pos()` (JS), com `se` limitado a [−1, 1] antes do arcsin para evitar erro numérico.

### 13.6 Azimute do sol

**Ideia física.** A direção da bússola de onde vem o sol (0°=Norte, 90°=Leste, 180°=Sul, 270°=Oeste).

**Fórmula.**

```
cos(A) = ( sin(δ) − sin(φ)·sin(el) ) / ( cos(φ)·cos(el) )
A = arccos( cos(A) )
se sin(H) > 0  →  A = 360° − A     (corrige tarde/manhã)
```

**Código.** `sunPos()` em `src/Sun.cpp` e `pos()` em `src/WebPortal.cpp`:

```c
float cA = (sinf(dec) - sinf(phi)*sinf(e)) / (cosf(phi)*cosf(e));
float A = acosf(cA)*DEGc;
if (sinf(H) > 0) A = 360 - A;
```

### 13.7 Nascer, pôr e meio‑dia solar

**Ideia física.** O sol nasce/se põe quando a elevação cruza o horizonte (el=0). Resolvendo a equação da elevação para `el=0` dá o ângulo horário do nascer/pôr `H0`.

**Fórmulas.**

```
cos(H0) = −tan(φ)·tan(δ)
H0 (horas) = arccos(cos H0) / 15
meio-dia = 12 − TC/60
nascer   = meio-dia − H0
pôr      = meio-dia + H0
duração  = 2 · H0
```

**Casos extremos (regiões polares).** Se `cos(H0) > 1` → o sol não nasce (noite polar); se `< −1` → não se põe (dia polar). O código testa isso e marca `valid = false` (C) / mostra a mensagem (JS).

**Código.** `Sun_Compute()` em `src/Sun.cpp`:

```c
float c = -tanf(phi)*tanf(dec);
o->valid = (c <= 1 && c >= -1);
float H0 = acosf(c)*DEGc/15.0f;
o->rise = noon - H0; o->set = noon + H0; o->dayLen = 2*H0;
```

e `times()` em `src/WebPortal.cpp` (mesma conta, com as mensagens "sol não nasce/se põe hoje").

### 13.8 Insolação na fachada

**Ideia física.** Saber por quanto tempo o sol bate **diretamente** numa parede orientada para um certo azimute (`facadeAz`). O sol bate na fachada quando está acima do horizonte **e** vindo de um lado em que a parede o "vê" — ou seja, quando a diferença angular entre o azimute do sol e o da fachada é menor que 90°.

**Fórmula.**

```
sol bate na fachada  ⟺  el > 0  E  cos(azimute_sol − azimute_fachada) > 0
```

O `cos(...) > 0` é exatamente a condição de a diferença de ângulo ser < 90°. O total é integrado varrendo o dia em passos de 1 minuto.

**Código.** Laço em `Sun_Compute()` (`src/Sun.cpp`):

```c
for (float t = o->rise; t <= o->set; t += 1.0f/60.0f) {
    float e2, a2; sunPos(N, t, &e2, &a2);
    if (e2 > 0 && cosf((a2 - sunP.facadeAz)*RAD) > 0) {
        o->facHours += 1.0f/60.0f; ...
    }
}
```

e o mesmo laço em JS (`calc()` em `src/WebPortal.cpp`), que ainda guarda a janela de horários (`win[]`).

### 13.9 Sombra ao meio‑dia

**Ideia física.** O comprimento da sombra de um objeto vertical de altura `h` depende de quão alto está o sol: quanto mais baixo o sol, mais longa a sombra. É trigonometria pura.

**Fórmula.**

```
sombra = altura / tan(elevação)
```

(só faz sentido com o sol acima do horizonte; no código, `el > 1°`).

**Código.** `Sun_Compute()` em `src/Sun.cpp`:

```c
o->shadowNoon = (el > 1) ? (sunP.objH / tanf(el*RAD)) : -1;
```

e em JS (`calc()` em `src/WebPortal.cpp`): `sh = eln>1 ? hh/Math.tan(eln*R) : null`.

### 13.10 Desenho da carta solar

A função `Sun_Path()` (`src/Sun.cpp`) gera N pontos `(azimute, elevação)` ao longo do dia, do nascer ao pôr, para desenhar a curva. Na página web, a função `chart()` desenha o mesmo em **SVG** (eixo X = azimute 0–360° com marcas N/L/S/O, eixo Y = altura 0–90°), com uma linha tracejada laranja marcando a orientação da fachada. Os parâmetros padrão (São Paulo) estão em `sunP = { -23.55, -46.63, -3.0, 0.0, 3.0 }` (lat, lon, fuso, fachada, altura).

### 13.11 Beiral para sombrear a janela (web)

**Ideia física.** Para barrar o sol do meio‑dia (o mais quente) numa janela, projeta‑se um beiral horizontal acima dela. Quanto mais alto o sol ao meio‑dia (`elN`), mais curto basta o beiral para sombrear 1 m de janela — é a inversa da conta de sombra (item 13.9, com altura = 1 m).

**Fórmula.**

```
beiral (por m de janela) = 1 / tan(elevação_meio-dia)
```

(só é exibido quando o sol ao meio‑dia está razoavelmente alto, `elN > 5°`; abaixo disso o beiral ficaria absurdamente longo e mostra `--`).

**Código.** `calc()` em `src/WebPortal.cpp`:

```js
const eln = pos(lat,lon,tz,N,T.noon).el;          // elevacao ao meio-dia
... eln>5 ? (1/Math.tan(eln*R)).toFixed(2)+' m' : '--'
```

### 13.12 Painel solar — inclinação ótima (web)

**Ideia física.** Para o ano todo, a regra prática é inclinar o painel num ângulo igual à **latitude do local**, voltado para o **equador** (Norte no hemisfério Sul, Sul no hemisfério Norte). Assim o painel fica, em média, mais perpendicular ao sol.

**Fórmula (regra prática).**

```
inclinação ótima ≈ | latitude |
orientação       = Norte  (se lat < 0)  /  Sul  (se lat > 0)
```

**Código.** `calc()` em `src/WebPortal.cpp`:

```js
~${Math.abs(lat).toFixed(0)}° p/ ${lat<0?'Norte':'Sul'}
```

É uma **aproximação** (não considera ajuste sazonal nem perdas atmosféricas); serve de ponto de partida para instalação.

### 13.13 Melhor orientação dos ambientes (web)

A mesma lógica de hemisfério dá dicas de projeto, sem fórmula nova:

| Item | Hemisfério Sul (lat < 0) | Hemisfério Norte (lat > 0) |
|---|---|---|
| Melhor face (sol o dia todo) | Norte | Sul |
| Sol da manhã / da tarde | Leste / Oeste | Leste / Oeste |
| Face mais fresca (depósito) | Sul | Norte |

**Código.** `calc()` em `src/WebPortal.cpp` (ternários `lat<0?...`).

### 13.14 Máscara de sombreamento — horas de sol por mês (web)

**Ideia física.** Um gráfico de 12 barras (uma por mês) mostrando **quantas horas por dia** o sol bate na fachada escolhida, no dia 15 de cada mês. Revela a sazonalidade da insolação da parede ao longo do ano.

**Como é calculado.** Para cada mês toma‑se o dia 15 (`N = dia_cumulativo + 15`), varre‑se o dia do nascer ao pôr em passos de 2 minutos e soma‑se o tempo em que o sol está acima do horizonte **e** vindo de um lado que a fachada "vê" (mesma condição da insolação na fachada, item 13.8):

```
para cada mês m (dia 15):
    horas = Σ (2/60)  para t de rise…set, passo 2 min,
            enquanto  el > 0  E  cos(azimute_sol − fachada) > 0
```

A altura de cada barra é normalizada pelo mês de maior insolação (`max(hrs)`).

**Código.** `maskBars()` em `src/WebPortal.cpp`:

```js
for (let m=0; m<12; m++){ let N=cum[m]+15, T=times(lat,lon,tz,N), h=0;
  if(!T.none){ for(let t=T.rise; t<=T.set; t+=2/60){
    let p=pos(lat,lon,tz,N,t);
    if(p.el>0 && Math.cos((p.az-fac)*R)>0) h+=2/60; } }
  hrs.push(h); }
```

(É a mesma física dos itens 13.5–13.8, repetida mês a mês; meses de noite/dia polar — `T.none` — entram com 0 h.)

---

## 🔌 14. Como foi feita a conexão (hardware)

Tudo roda numa **placa única** — a Waveshare ESP32‑S3‑Touch‑LCD‑1.46B. Não há fios externos nem módulos separados: o display, o toque, o IMU, o RTC, o microfone, a bateria e o expansor já vêm integrados e ligados ao ESP32‑S3 por três barramentos.

### Barramentos

| Barramento | Para quê | Pinos (GPIO) |
|---|---|---|
| **QSPI** | Tela (display SPD2010), rápido, 4 linhas de dados | SCK=40, D0=46, D1=45, D2=42, D3=41, CS=21, TE=18 |
| **I2C** (400 kHz) | Compartilhado: IMU, toque, RTC, expansor | SDA=11, SCL=10 |
| **I2S** | Microfone MEMS digital | BCK=15, WS=2, DIN=39 |

### Endereços no barramento I2C

Como o I2C é um barramento compartilhado, cada periférico tem um endereço único:

| Dispositivo | Endereço | Papel |
|---|---|---|
| IMU **QMI8658** | 0x6B | Acelerômetro/giroscópio (todos os ângulos) |
| Toque **SPD2010** | 0x53 | Touch capacitivo (INT=GPIO4) |
| RTC **PCF85063** | 0x51 | Relógio de tempo real (data/hora p/ o sol) |
| Expansor **TCA9554** | 0x20 | Saídas de controle (ver abaixo) |

### Outros pinos

- **Bateria:** ADC em **GPIO8** (lê a tensão para estimar carga, driver `BAT_Driver`).
- **Tecla liga/desliga:** entrada **GPIO6**, controle **GPIO7** (driver `PWR_Key`).
- **Display TE:** **GPIO18** (tearing effect, sincroniza o refresh sem rasgar a imagem).

### Papel do expansor TCA9554

O ESP32‑S3 não tem pinos sobrando para tudo, então um **expansor de portas I/O TCA9554PWR** (no endereço I2C 0x20) cria pinos extras controlados via I2C. Ele cuida dos sinais de controle "lentos" mas essenciais:

- **RESET do display** e **RESET do touch** (pulsos de inicialização);
- **Backlight** (liga/ajusta a luz de fundo da tela).

Assim, com 2 fios de I2C (SDA/SCL) o firmware consegue resetar e acender a tela sem gastar GPIOs dedicados do ESP. O driver é `TCA9554PWR`.

### Camada de software (drivers)

A leitura/escrita real é feita pelos drivers Waveshare: `Display_SPD2010` + `esp_lcd_spd2010` + `LVGL_Driver` (tela e interface gráfica LVGL), `Touch_SPD2010` (toque), `Gyro_QMI8658` (IMU), `RTC_PCF85063` (relógio), `I2C_Driver` (barramento), `TCA9554PWR` (expansor), `BAT_Driver` e `PWR_Key`.

---

## 📶 15. Como funciona o WiFi / servidor

**Ideia.** O celular não precisa de internet: a própria placa **cria uma rede WiFi** (modo Access Point) e serve as páginas. Tudo é offline.

**Como conectar.**

- Rede: **`Medidor-Obra`**
- Senha: **`belasartes`** (WPA2, ≥ 8 caracteres)
- Endereço no navegador: **`http://192.168.4.1`**

(Definidos em `src/WebPortal.h`: `AP_SSID`, `AP_PASS`, `AP_URL`.)

**Como o código faz** — `src/WebPortal.cpp`, `WebPortal_Init()`:

```c
WiFi.mode(WIFI_AP);
WiFi.softAP(AP_SSID, AP_PASS);
... server.on(...) ...
server.begin();
```

O servidor HTTP roda na porta 80. No loop principal, `WebPortal_Loop()` chama `server.handleClient()` para atender as requisições.

### Rotas disponíveis

| Rota | Método | O que faz |
|---|---|---|
| `/` | GET | Página principal (leitura ao vivo + tabela de medições) |
| `/sol` | GET | Página da carta solar (todo o cálculo roda no navegador) |
| `/live` | GET → JSON | Valor atual da ferramenta ativa: `{mode, value, unit}` |
| `/data` | GET → JSON | Lista das medições salvas |
| `/data.csv` | GET | Baixa as medições em CSV (`medicoes.csv`) |
| `/clear` | GET | Apaga todas as medições |
| `/settime` | GET | Acerta o RTC com a hora do celular (`y, mo, d, h, mi, s, w`) |
| `/setsol` | GET | Recebe local/fachada do celular e grava em `sunP` (lat, lon, tz, fac, h) |

### Detalhes importantes

- **Página `/` (ao vivo).** O JavaScript faz `fetch('/live')` a cada 1,5 s e `fetch('/data')` a cada 3 s, atualizando a tela do celular sem recarregar.
- **Acertar relógio.** O botão lê `new Date()` do celular e envia para `/settime`, que escreve no RTC PCF85063 via `PCF85063_Set_All()`. Assim os cálculos solares usam a data/hora corretas mesmo sem internet.
- **Página `/sol`.** Todo o cálculo astronômico (itens 13.x) roda **no navegador**, em JavaScript — a placa não precisa calcular nada para essa página. Recursos extras: **GPS do celular** (`navigator.geolocation`) e **bússola** (`DeviceOrientation`) para preencher latitude/longitude e a orientação da fachada automaticamente.
- **Botão "Mostrar na tela do ESP".** Envia os parâmetros para `/setsol`, que atualiza a struct global `sunP`. Aí a placa passa a desenhar a carta solar na própria tela (menu SOL) com o mesmo local/fachada escolhidos no celular — garantindo que a versão em C (`src/Sun.cpp`) e a versão em JS produzam o mesmo resultado.

---

## 📚 Resumo dos arquivos citados

| Arquivo | Conteúdo |
|---|---|
| `src/LevelApp.cpp` | Ângulos (roll/pitch), filtro, nível, prumo, declividade %, transferidor, conversor, esquadro, planeza, perfil, normas, calibração `gCalR/gCalP`, bateria % |
| `src/Mic_dB.cpp` | Decibelímetro: DC, RMS, dB, calibração `SPL_REF` |
| `src/BAT_Driver.cpp` | Leitura da tensão da bateria (ADC GPIO8 → volts) |
| `src/Sun.cpp` / `src/Sun.h` | Astronomia solar em C (tela do ESP) |
| `src/WebPortal.cpp` / `src/WebPortal.h` | AP WiFi + servidor HTTP + páginas HTML/JS (conta solar em JS: beiral, painel solar, máscara de sombreamento) |

---

*Autores: Murillo Vinicius (@dantazzzz) e Amanda Célia Aparecida da Silva — Belas Artes (FEBASP).
Licença CC BY‑NC 4.0.*
