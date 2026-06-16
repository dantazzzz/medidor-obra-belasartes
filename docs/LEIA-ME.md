# 📐 Medidor de obra — ESP32-S3-Touch-LCD-1.46B (Arduino IDE)

Nível digital · Prumo · Declividade (%) · Transferidor — tudo pelo acelerômetro
(QMI8658) na tela redonda de 412×412.

## ✅ Já está pronto e COMPILADO
Montei o sketch completo (drivers da Waveshare + meu medidor) **nesta pasta** e
**compilei aqui** com seu `esp32 core 3.3.3` + `LVGL 8.3.10`:
> Sketch: **693 KB (22%)** · RAM global: **43%** — compila limpo, sobra espaço.

Você **não precisa baixar nem editar nada** — é só abrir, configurar a placa e gravar.

---

## 1) Abrir
No Arduino IDE: **File → Open** → abra **`NivelDigital.ino`** (nesta pasta).

## 2) Configurar a placa (Tools) — IMPORTANTE, use exatamente isto
Foi com essa config que compilou aqui:

| Opção (Tools) | Valor |
|---|---|
| Board | **ESP32S3 Dev Module** |
| PSRAM | **OPI PSRAM** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** |
| USB CDC On Boot | **Enabled** |
| USB Mode | **Hardware CDC and JTAG** |
| Upload Speed | **921600** |

## 3) Gravar
1. Conecte a placa no USB-C, **Tools → Port** → selecione a porta que apareceu.
2. Clique **Upload** (→).
   - Se não gravar: segure **BOOT**, toque **RESET**, solte **BOOT**, e grave de novo.
3. Deve aparecer o **medidor** na tela. 🎉

---

## Como usar
Botões na base (toque):
- **MODO** — cicla: **NIVEL → PRUMO → DECLIVIDADE → TRANSFERIDOR → RUIDO**
- **ZERAR** — zera a referência (ou, no modo RUIDO, **reseta o min/max**)
- **HOLD** — congela a leitura pra anotar

| Modo | Pra que serve |
|---|---|
| **NIVEL** | bolha 2D — nivelar piso/laje |
| **PRUMO** | verticalidade de parede/pilar |
| **DECLIVIDADE** | caimento em **%** com **checagem de norma** (ver abaixo) |
| **TRANSFERIDOR** | ângulo relativo desde o último ZERAR |
| **RUIDO** 🆕 | **decibelímetro** (mic da placa) — dB + min/max |

### 🆕 DECLIVIDADE com presets de norma
No modo DECLIVIDADE aparece um rótulo azul no topo (ex.: `RAMPA NBR9050 ate 8.3%`).
**Toque nele pra trocar** o preset: rampa acessível, piso p/ ralo, esgoto, água
pluvial, laje, ou "livre". O número fica **verde (DENTRO DA NORMA)** ou **vermelho
(FORA)**. *(São valores-guia — confirme a norma do seu caso.)*

### 🆕 RUIDO (decibelímetro)
Mostra o nível sonoro em **dB**: verde < 70, amarelo 70–85, vermelho > 85
(referência NR-15). Embaixo, **min/max**. **ZERAR** reseta o min/max.
> ⚠️ **É aproximado e precisa de calibração.** Compare com um app de decibelímetro
> do celular e ajuste `SPL_REF` em `Mic_dB.cpp` (cada **+1** sobe ~1 dB na leitura).

> **Bolha pro lado errado?** Abra `LevelApp.cpp`, lá no topo troque `SIGN_X` /
> `SIGN_Y` (de `+1.0f` pra `-1.0f`). Recompile.

---

## O que eu mexi no seu ambiente (pra você saber)
- **`libraries/lvgl/src/lv_conf.h`** — esse era o `lv_conf` que valia nos seus projetos
  (vinha de um setup Waveshare anterior). As fontes grandes 14/20/22/40 estavam
  **desligadas**; liguei elas (o medidor precisa). Isso **não quebra** seus outros
  projetos — só deixa mais fontes disponíveis (+uns KB de flash).
  Backup do original em `libraries/lvgl/src/lv_conf.h.bak`.
- Mais nada foi alterado. Seu LVGL continua 8.3.10, sua PlatformIO está intacta.

## Se der erro
Compilou aqui, então deve gravar direto. Se aparecer algo na hora do Upload
(porta/driver USB), me manda a mensagem que eu resolvo.
