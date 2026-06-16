# 📐 Medidor de Obra — Belas Artes

**Cinco ferramentas de obra de bolso numa telinha redonda sensível ao toque
(ESP32-S3):** nível, prumo, declividade (%), transferidor e decibelímetro — tudo
calculado a partir dos sensores que já vêm na placa. Você ainda **salva as
medições** com data/hora, abre um **WiFi próprio** para ver e baixar tudo no
celular, e calcula a **insolação solar** (carta solar) de qualquer local —
**100% offline, sem internet**.

Firmware Arduino (C/C++) para a placa **Waveshare ESP32-S3-Touch-LCD-1.46B**
(tela redonda AMOLED 412×412 com toque, IMU QMI8658, microfone I2S, relógio
PCF85063 e WiFi).

Desenvolvido por **Murillo Vinícius** e **Amanda Célia Aparecida da Silva** ·
Centro Universitário **Belas Artes** de São Paulo (FEBASP).

> 👉 Não é da área de eletrônica/programação? Comece pelo
> **[docs/GUIA.md](docs/GUIA.md)** — explica tudo em linguagem simples.

---

## ✨ O que faz

As **5 ferramentas** ficam no menu (arraste de lado / *swipe*). Cada uma mostra a
leitura grande no centro, com botões **MENU · ZERAR · HOLD · SALVAR**.

| Ferramenta | Para que serve | Unidade |
|---|---|---|
| **Nível** | bolha 2D para nivelar piso, laje, móvel | graus |
| **Prumo** | conferir se parede/pilar está na vertical | graus |
| **Declividade (%)** | caimento de rampa, telhado, tubo, calha — com **checagem de norma** (toque para trocar) que fica **verde (dentro)** ou **vermelho (fora)** | % |
| **Transferidor** | medir o ângulo entre duas superfícies | graus |
| **Ruído (dB)** | decibelímetro pelo microfone da placa, com **mín/máx** e faixas inspiradas na NR-15 | dB |

E mais dois cartões no menu:

- **🌞 SOL (carta solar):** desenha a **cúpula do céu** na própria tela do ESP,
  com o caminho do sol no dia e a posição do sol agora (lida do relógio interno).
- **📶 DADOS / WiFi:** instruções na tela para conectar o celular e ver/baixar as
  medições.

### Checagem de norma na declividade

A ferramenta de declividade traz *presets* prontos (toque no texto azul para
trocar). São **guias** — confira sempre a norma vigente:

| Preset | Limite |
|---|---|
| Rampa NBR 9050 | até 8,33 % |
| Piso p/ ralo | 0,5 – 2 % |
| Esgoto 100 mm | mín. 1 % |
| Água pluvial | mín. 0,5 % |
| Laje impermeabilizada | mín. 1 % |
| Livre | sem limite |

---

## 🧰 Hardware

**Placa: Waveshare ESP32-S3-Touch-LCD-1.46B** — nenhum sensor extra é necessário,
tudo usa o que já vem na placa.

| Recurso | Componente | Papel no projeto |
|---|---|---|
| Tela redonda AMOLED 412×412 + toque | SPD2010 (barramento QSPI) | toda a interface |
| Acelerômetro + giroscópio | IMU QMI8658 (I²C `0x6B`) | vira nível, prumo, declividade e transferidor |
| Microfone MEMS | I2S | vira o decibelímetro |
| Relógio de tempo real | PCF85063 (I²C `0x51`) | carimba as medições com data/hora |
| Expansor de I/O | TCA9554 (I²C `0x20`) | reset da tela/toque e *backlight* |
| Conectividade | WiFi/Bluetooth, PSRAM 8 MB | portal no celular |

![Mapa de conexoes](docs/diagrama_conexoes.svg)

O que a placa **não** tem (bússola, GPS) é fornecido pelo **celular**, através da
página web do Sol.

---

## 🚀 Como gravar

O passo a passo completo (Arduino IDE, configuração da placa, biblioteca) está em
**[docs/LEIA-ME.md](docs/LEIA-ME.md)**. Resumo:

1. **Arduino IDE 2.x** + pacote **`esp32` da Espressif (3.x)**.
2. Biblioteca **LVGL 8.3.x** + o `lv_conf.h` deste projeto.
3. Abrir `NivelDigital.ino`, selecionar **ESP32S3 Dev Module** com **OPI PSRAM /
   16 MB / USB CDC On Boot: Enabled** e clicar em **Upload**.

---

## 🏗️ Como funciona (arquitetura)

```
NivelDigital.ino     -> bring-up da placa, sobe as tarefas e roda o loop principal
 ├─ AppUi.*          -> telas: SPLASH (logo) -> MENU (carrossel/swipe) -> ferramenta
 ├─ LevelApp.*       -> as 5 ferramentas (matemática do ângulo + UI da medição)
 ├─ SunApp.*         -> carta solar desenhada na tela (cúpula do céu polar)
 ├─ Mic_dB.*         -> decibelímetro (lê o microfone I2S e calcula o nível em dB)
 ├─ DataLog.*        -> registro das medições salvas (carimbado com a hora do RTC)
 ├─ WebPortal.*      -> cria o WiFi e serve as páginas (dados + /sol)
 ├─ Sun.*            -> astronomia solar em C (a mesma da página web, para a tela)
 ├─ logo_belasartes.c-> o brasão exibido no splash
 └─ drivers Waveshare-> tela, toque, IMU, relógio, I²C, energia, bateria
```

Os **sensores** rodam numa *task* separada no core 0 (`Sensor_Task`, atualiza IMU,
RTC, bateria e botão de energia), enquanto a **interface LVGL** roda toda no
`loop()` no core 1 — porque a LVGL não é *thread-safe*.

**A medição do ângulo** vem do acelerômetro: parado, ele "sente" a gravidade, e a
direção dessa força revela a inclinação:

```
roll  = atan2(ay, az)
pitch = atan2(-ax, √(ay² + az²))
```

A leitura passa por um filtro suave e a declividade em % é `tan(ângulo) × 100`.

**O decibelímetro** lê blocos do microfone I2S, remove o nível DC, calcula o RMS e
converte para dB (`20·log10(rms/32768) + SPL_REF`). É aproximado e calibrável pela
constante `SPL_REF` em `Mic_dB.cpp` (cada +1 sobe ~1 dB na leitura).

**O cálculo solar** existe em dois lugares com a mesma matemática (declinação,
equação do tempo, altura e azimute do sol): em **JavaScript no navegador** (página
`/sol`) e em **C** (`Sun.cpp`) para desenhar a carta solar na tela do ESP.

> Detalhe: a tela é **redonda**, então toda a interface fica dentro de um círculo
> seguro — cantos e bordas são cortados pelo vidro.

---

## 📶 Portal WiFi e a página do Sol

A placa cria a própria rede WiFi (não precisa de roteador):

- **Rede:** `Medidor-Obra`
- **Senha:** `belasartes`
- **Endereço:** `http://192.168.4.1`

**Página principal (`/`)** — leitura **ao vivo** da ferramenta aberta, **tabela**
das medições salvas, **download CSV**, botão **Limpar** e **Acertar relógio** (usa
a hora do próprio celular, via `/settime`).

**Página do Sol (`/sol`)** — todo o cálculo de astronomia roda **offline, no
navegador do celular**. Você informa latitude/longitude, fuso, data, a orientação
da fachada e a altura para a sombra, e a página entrega:

- **Carta solar** (gráfico azimute × altura do sol) com a linha da fachada;
- **Nascer / pôr do sol**, **meio-dia solar** e **duração do dia**;
- **Altura máxima do sol**;
- **Horas de sol direto na fachada** e a **janela de insolação**;
- **Comprimento da sombra ao meio-dia** para uma dada altura.

Atalhos: **Usar GPS do celular**, **Bússola do cel** (ajusta a fachada) e o botão
**Mostrar na tela do ESP** — que envia o local/fachada para a placa (rota
`/setsol`) e desenha a carta solar na própria tela.

---

## 📁 Estrutura de pastas

```
NivelDigital/
├─ NivelDigital.ino        setup() / loop()
├─ src/                    o aplicativo + drivers da placa
│  ├─ AppUi.*              telas e navegação (LVGL 8)
│  ├─ LevelApp.*           as 5 ferramentas + matemática do acelerômetro
│  ├─ SunApp.* · Sun.*     carta solar na tela + astronomia em C
│  ├─ Mic_dB.*             decibelímetro (I2S)
│  ├─ DataLog.*            registro das medições
│  ├─ WebPortal.*          AP WiFi + páginas HTML (dados e /sol)
│  ├─ logo_belasartes.c    brasão do splash
│  └─ drivers Waveshare    Display/Touch SPD2010, I2C, TCA9554, LVGL_Driver,
│                          QMI8658, PCF85063, PWR, BAT...
├─ docs/                   GUIA.md · LEIA-ME.md · diagrama_conexoes.svg
└─ tools/                  utilitários (logo/ — gerador do brasão)
```

📖 **Saiba mais:** [docs/COMO_FUNCIONA.md](docs/COMO_FUNCIONA.md) (detalhes
técnicos) · [docs/GUIA.md](docs/GUIA.md) (guia para leigos) ·
[docs/LEIA-ME.md](docs/LEIA-ME.md) (como gravar).

---

## 🙏 Créditos

**Autores:**
- **Murillo Vinícius** — GitHub [@dantazzzz](https://github.com/dantazzzz) · murillovinicius18@gmail.com
- **Amanda Célia Aparecida da Silva**

**Instituição:** Centro Universitário Belas Artes de São Paulo (FEBASP).

**Tecnologias de terceiros:**
- Drivers da placa: demos oficiais da [Waveshare](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.46B)
- Interface gráfica: [LVGL](https://lvgl.io) 8.3 (licença MIT)

---

## 📜 Licença

**CC BY-NC 4.0** — uso livre para fins **não comerciais**, desde que **citada a
autoria** (Murillo Vinícius e Amanda Célia Aparecida da Silva). Uso comercial
requer autorização. Ver [LICENSE](LICENSE).
