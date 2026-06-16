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

## ☀️ 7. Carta solar / Insolação (astronomia solar offline)

Esta é a parte mais matemática. O **mesmo cálculo existe em dois lugares**: em **C** (`src/Sun.cpp`, para desenhar na tela do ESP) e em **JavaScript** (página `/sol` dentro de `src/WebPortal.cpp`, para rodar no celular). As fórmulas são idênticas; abaixo elas aparecem uma vez, com referência aos dois arquivos.

### 7.1 Dia do ano (N)

Número do dia no ano (1 a 365/366), com correção de ano bissexto. É a entrada de quase tudo.

- **C** — `dayOfYear()` em `src/Sun.cpp` (tabela cumulativa de meses + ajuste de bissexto).
- **JS** — `doy()` em `src/WebPortal.cpp` (diferença de datas UTC).

### 7.2 Declinação solar (δ)

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

### 7.3 Equação do tempo (EoT)

**Ideia física.** O sol "verdadeiro" adianta ou atrasa em relação ao relógio (até ~±15 min) por causa da órbita elíptica e da inclinação do eixo. A EoT corrige isso.

**Fórmula.**

```
B   = 360° · (N − 81) / 364
EoT = 9.87·sin(2B) − 7.53·cos(B) − 1.5·sin(B)   [minutos]
```

**Origem.** Aproximação clássica de Spencer/duffie da equação do tempo.

**Código.** `eotN()` em `src/Sun.cpp` e `eot()` em `src/WebPortal.cpp`.

### 7.4 Correção de tempo e hora solar

**Ideia física.** Converter a hora do relógio (no fuso `tz`) para a **hora solar local** real do ponto (longitude `lon`), combinando o deslocamento de longitude com a EoT.

**Fórmulas.**

```
TC  = 4 · (lon − 15·tz) + EoT        [minutos]   (4 min por grau de longitude)
LST = hora_relógio + TC/60           [hora solar local]
H   = 15° · (LST − 12)               [ângulo horário; 15°/h, 0 ao meio-dia]
```

**Código.** Dentro de `sunPos()` em `src/Sun.cpp` e `pos()` em `src/WebPortal.cpp`.

### 7.5 Elevação (altura) do sol

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

### 7.6 Azimute do sol

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

### 7.7 Nascer, pôr e meio‑dia solar

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

### 7.8 Insolação na fachada

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

### 7.9 Sombra ao meio‑dia

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

### 7.10 Desenho da carta solar

A função `Sun_Path()` (`src/Sun.cpp`) gera N pontos `(azimute, elevação)` ao longo do dia, do nascer ao pôr, para desenhar a curva. Na página web, a função `chart()` desenha o mesmo em **SVG** (eixo X = azimute 0–360° com marcas N/L/S/O, eixo Y = altura 0–90°), com uma linha tracejada laranja marcando a orientação da fachada. Os parâmetros padrão (São Paulo) estão em `sunP = { -23.55, -46.63, -3.0, 0.0, 3.0 }` (lat, lon, fuso, fachada, altura).

---

## 🔌 8. Como foi feita a conexão (hardware)

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

## 📶 9. Como funciona o WiFi / servidor

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
- **Página `/sol`.** Todo o cálculo astronômico (itens 7.x) roda **no navegador**, em JavaScript — a placa não precisa calcular nada para essa página. Recursos extras: **GPS do celular** (`navigator.geolocation`) e **bússola** (`DeviceOrientation`) para preencher latitude/longitude e a orientação da fachada automaticamente.
- **Botão "Mostrar na tela do ESP".** Envia os parâmetros para `/setsol`, que atualiza a struct global `sunP`. Aí a placa passa a desenhar a carta solar na própria tela (menu SOL) com o mesmo local/fachada escolhidos no celular — garantindo que a versão em C (`src/Sun.cpp`) e a versão em JS produzam o mesmo resultado.

---

## 📚 Resumo dos arquivos citados

| Arquivo | Conteúdo |
|---|---|
| `src/LevelApp.cpp` | Ângulos (roll/pitch), filtro, nível, prumo, declividade %, transferidor, normas |
| `src/Mic_dB.cpp` | Decibelímetro: DC, RMS, dB, calibração `SPL_REF` |
| `src/Sun.cpp` / `src/Sun.h` | Astronomia solar em C (tela do ESP) |
| `src/WebPortal.cpp` / `src/WebPortal.h` | AP WiFi + servidor HTTP + páginas HTML/JS (mesma conta solar em JavaScript) |

---

*Autores: Murillo Vinicius (@dantazzzz) e Amanda Célia Aparecida da Silva — Belas Artes (FEBASP).
Licença CC BY‑NC 4.0.*
