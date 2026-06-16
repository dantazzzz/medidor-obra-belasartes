# 📖 Guia para todos — o Medidor de Obra explicado sem mistério

Este guia é para **quem não é da área** de eletrônica nem de programação. Sem termos
complicados — e, quando algum aparecer, ele vem traduzido logo em seguida. 🙂

A ideia é que, ao terminar de ler, você saiba **o que o aparelho faz**, **como usar
cada botão**, **como conectar no celular** e até **como ele funciona por dentro** —
tudo com analogias do dia a dia.

---

## 🤔 O que é, em uma frase

É uma **telinha redonda de mão** que mede coisas de obra — **se algo está reto, no
prumo, a inclinação de uma rampa, o ângulo de um canto e o barulho do ambiente** — e
que ainda **conversa com o seu celular por WiFi** e **calcula o caminho do sol** num
lugar e numa data quaisquer.

Pense num **nível de pedreiro** (aquele com a bolhinha de água), só que **digital**,
com tela, e fazendo o trabalho de **várias ferramentas ao mesmo tempo** — um verdadeiro
"canivete suíço" do canteiro de obras.

---

## 👀 Como usar — passo a passo

### 1. Ligar

Ligue a placa pelo cabo **USB-C** (a um carregador, computador ou bateria portátil).
Aparece o **brasão da Belas Artes** — é a **tela de abertura** (em inglês, *splash*).
Ela fica uns **2 segundos e meio** e sozinha dá lugar ao **menu**.

> 💡 *Analogia:* é como a marca que pisca quando você liga a TV antes de aparecer a
> programação.

### 2. Escolher a ferramenta no menu

O menu é um **carrossel**: vários cartões lado a lado. Você **arrasta o dedo para o
lado** (igual a trocar de foto no celular) e os cartões deslizam, parando sempre um no
centro. Os **pontinhos** embaixo mostram em qual cartão você está. Para entrar,
**toque** no cartão do meio.

São **7 cartões** no total — as 5 ferramentas de medição mais o Sol e os Dados/WiFi:

| Cartão | O que faz |
|---|---|
| **NÍVEL** | Bolha em 2D: mostra se uma superfície deitada está reta (chão, mesa, peitoril). |
| **PRUMO** | Verticalidade: mostra se uma parede ou um poste está "em pé" certinho. |
| **DECLIVIDADE** | Caimento em **porcentagem (%)** — para rampa, telhado, cano, ralo. |
| **TRANSFERIDOR** | Mede um **ângulo** qualquer (abertura de um canto, de uma tampa). |
| **RUÍDO** | Mede o **barulho** do ambiente em **decibéis (dB)** pelo microfone da placa. |
| **SOL** | Desenha a **carta solar** na própria tela (o caminho do sol no céu). |
| **DADOS** | Mostra a senha do **WiFi** e o endereço para abrir tudo no **celular**. |

### 3. Os 4 botões dentro de cada ferramenta

Em qualquer uma das 5 ferramentas de medição aparecem **4 botões** na parte de baixo:

| Botão | Cor | O que faz |
|---|---|---|
| **MENU** | cinza | Volta para o menu (o carrossel). |
| **ZERAR** | azul | Define a posição atual como o "zero". No Ruído, **reinicia o mínimo e o máximo**. |
| **HOLD** | escuro | **Congela** o número na tela para você ler com calma. Aperte de novo para descongelar. |
| **SALVAR** | verde | **Guarda** a medição atual numa lista (aparece "SALVO" por um instante). |

> 💡 **Para que serve o ZERAR?** Imagine medir o caimento de um cano que está sobre uma
> mesa **já um pouco torta**. Você apoia a placa na mesa, aperta **ZERAR** (ela passa a
> chamar aquilo de "reto") e depois mede o cano: o resultado mostra **só** a inclinação
> do cano, sem o erro da mesa. É como **tarar uma balança** antes de pesar.

### 4. Detalhe de cada ferramenta

- **Nível** — deite a placa na superfície. Uma **bolinha verde** anda dentro do círculo:
  quando fica **bem no centro** (e o número chega perto de zero), aparece **"NIVELADO"**.
- **Prumo** — encoste a placa **em pé** na parede. Quando estiver vertical de verdade,
  aparece **"PRUMO OK"**.
- **Declividade** — mostra o caimento em **%**. Há um aviso de **norma** logo abaixo do
  número (o texto azul). **Tocando nesse texto** você troca entre 6 referências comuns,
  e a cor avisa se está **dentro** (verde) ou **fora** (vermelho):

  | Referência | Faixa que ela considera correta |
  |---|---|
  | Rampa NBR 9050 | até **8,33%** |
  | Piso para ralo | **0,5% a 2%** |
  | Esgoto (cano 100 mm) | mínimo **1%** |
  | Água pluvial (chuva) | mínimo **0,5%** |
  | Laje impermeabilizada | mínimo **1%** |
  | Livre | sem limite (só mostra o número) |

  > ⚠️ Esses valores são um **lembrete prático**, não substituem a leitura da norma
  > oficial e do projeto. Sempre confira a norma vigente.

- **Transferidor** — mede o **ângulo** de inclinação combinando os dois sentidos da placa.
- **Ruído** — mostra o som em **dB** e dá um parecer por cor, inspirado na ideia da
  norma de ruído ocupacional **NR-15**:

  | Leitura | Aviso | Cor |
  |---|---|---|
  | abaixo de **70 dB** | OK | verde |
  | **70 a 85 dB** | ATENÇÃO | amarelo |
  | **85 dB ou mais** | ALTO (NR-15) | vermelho |

  Ele também guarda o **mínimo e o máximo** que ouviu — útil para flagrar um pico de
  barulho. O **ZERAR** reinicia esse mínimo/máximo.

---

## 📱 A parte do celular (WiFi)

A placa cria uma **rede WiFi própria**, como se fosse um **mini-roteador de bolso**.
Você não precisa de internet de verdade: o celular conecta **direto na placa**.

1. No celular, abra o WiFi e conecte na rede **`Medidor-Obra`** (senha **`belasartes`**).
2. Abra o **navegador** (Chrome, Safari) e digite **`192.168.4.1`** na barra de endereço.
3. Pronto. Aparece uma página com:
   - a **leitura ao vivo** (o que a placa está medindo agora, atualizando sozinha);
   - a **lista das medições que você salvou**, cada uma com o **horário**;
   - um botão **"Baixar CSV"** — salva tudo numa planilha (`medicoes.csv`, abre no Excel
     ou Google Sheets);
   - um botão **"Acertar relógio"** — coloca a hora do **seu celular** dentro da placa
     (assim os horários das medições ficam certos);
   - um botão **"Limpar"** — apaga a lista de medições.

> 🔎 O cartão **DADOS** na tela do aparelho serve só para te lembrar essa senha e esse
> endereço — não precisa decorar nada.

### A página do Sol no celular

Na página inicial há um botão grande **"Sol / Insolação"**. Nela você informa **onde**
(a latitude e a longitude do lugar) e **quando** (a data), e ela mostra:

- a hora do **nascer e do pôr do sol** e do **meio-dia solar** (sol mais alto);
- a **duração do dia** e a **altura máxima** que o sol alcança;
- **quantas horas de sol direto** uma parede (fachada) vai receber e em **qual janela
  de horário**;
- o **tamanho da sombra** que um objeto de certa altura projeta ao meio-dia;
- um **desenho da carta solar** (a curva que o sol descreve no céu naquele dia).

Para facilitar, há botões **"Usar GPS do celular"** (preenche o lugar sozinho) e
**"Bússola do cel"** (aponta o celular para a parede e ela ajusta a orientação da
fachada). E o botão **"Mostrar na tela do ESP"** **manda esses dados para o aparelho** —
aí a carta solar aparece **também na telinha redonda**, no cartão **SOL**.

> 💡 Isso é ouro para **arquitetura**: dá para descobrir a melhor orientação de uma
> casa, onde o sol bate de manhã ou à tarde, e o quanto uma janela vai pegar de sol ao
> longo do ano.

---

## 🧠 Como funciona "por dentro" (sem susto)

- **Como ela sabe a inclinação?** Dentro da placa há um sensorzinho chamado
  **acelerômetro**. Ele sente a **gravidade** — aquela força que puxa tudo para baixo.
  Em repouso, essa força sempre aponta para o chão; quando você **inclina** a placa, a
  direção dela muda **em relação ao aparelho**, e com um pouco de **cálculo de
  ângulos** (trigonometria) o programa transforma isso em graus. É a mesma ideia da bolha do nível de pedreiro, só
  que **com matemática no lugar do líquido**. A declividade em % é só esse ângulo
  visto de outro jeito: ela diz **quantos centímetros a superfície sobe a cada metro
  que se anda na horizontal**. Por exemplo, **2%** quer dizer "sobe 2 cm a cada
  metro".

- **Por que o número fica firme e não treme?** O programa aplica um **filtro de
  suavização**: em vez de obedecer a cada microtremida da sua mão, ele vai chegando ao
  valor aos poucos. É como **mexer um copo d'água devagar** para a superfície não ficar
  balançando.

- **Como ela mede o barulho?** Tem um **microfone** minúsculo (do tipo MEMS) embutido.
  O programa "escuta" pequenos pedaços de som muitas vezes por segundo, calcula a
  **força média da onda sonora** e converte isso para **decibéis (dB)**. Como cada
  microfone é um pouco diferente, esse número é uma **boa estimativa** — para precisão de
  laboratório seria preciso calibrar com um medidor profissional.

- **Como ela fala com o celular?** A placa tem **WiFi**, igual ao do seu roteador de
  casa. Em vez de **se conectar** a uma rede, ela **cria a própria rede** e ainda serve
  uma **página de site** — mas essa página mora **dentro da própria placa**. Por isso
  funciona sem internet nenhuma: é uma conversa direta entre celular e aparelho.

- **Como calcula o sol sem internet?** A posição do sol no céu é **totalmente
  previsível por astronomia**. Sabendo o **lugar** (latitude/longitude), o **fuso
  horário** e a **data**, dá para calcular com fórmulas a inclinação do sol, a que horas
  ele nasce e se põe, e por onde passa. O aparelho carrega essas fórmulas, então o
  cálculo roda **offline** — funciona até no meio do mato, sem sinal. (A página do
  celular faz a mesma conta lá, e o aparelho refaz na própria tela quando você manda os
  dados.)

- **As medições salvas, onde ficam?** Numa **lista na memória** da placa, com **carimbo
  de hora** vindo do **relógio interno**. Cabem as **últimas 60** medições; passou disso,
  a mais antiga sai para a nova entrar (como uma fila). Desligar o aparelho **apaga** a
  lista — por isso a opção de **baixar em planilha** pelo celular antes.

---

## 🛠️ Como foi feito (a história resumida)

1. **A placa.** Usamos uma plaquinha pronta da marca **Waveshare**, a **ESP32-S3** —
   um pequeno computador do tamanho de uma moeda, com **tela redonda colorida sensível
   ao toque** e vários sensores já embutidos (acelerômetro, microfone, relógio, WiFi).

2. **O programa.** Foi escrito no **Arduino IDE** — um programa **gratuito** de
   computador onde a gente digita o código e o "grava" na placa pela USB. A linguagem é
   o **C/C++**.

3. **A tela.** A interface (botões, bolinha, números, o menu que arrasta) foi montada
   com uma biblioteca gráfica chamada **LVGL**, feita justamente para telas pequenas.

4. **As ferramentas.** Cada modo (nível, prumo, declividade, transferidor, ruído) é um
   pedaço do programa que **pega a leitura do sensor**, faz a conta e **desenha o
   resultado** na tela.

5. **O WiFi e o sol.** A placa virou um **mini-servidor de site**; a página que aparece
   no celular foi feita com **HTML e JavaScript** (a "linguagem dos sites"). As fórmulas
   do sol existem nos **dois lados** — no celular e dentro do aparelho.

6. **O acabamento.** Logo da faculdade na abertura, menu em carrossel, e os botões
   posicionados para **caber numa tela redonda** (os cantos de uma tela redonda não
   existem, então tudo fica dentro de um círculo).

> 🧩 Resumindo as etapas: **escolher a placa → instalar o Arduino IDE e as bibliotecas
> → fazer os sensores funcionarem um a um → montar as telas no LVGL → criar o WiFi e a
> carta solar → ajustar tudo para a tela redonda e dar o acabamento.**

---

## 📚 Mini-glossário

- **ESP32-S3 / placa:** o "computadorzinho" que faz tudo funcionar.
- **Acelerômetro:** sensor que percebe inclinação e movimento (sente a gravidade).
- **MEMS:** tipo de sensor minúsculo gravado num chip — é como é o microfone da placa.
- **Firmware:** o programa que fica gravado **dentro** da placa.
- **Arduino IDE:** o programa de computador onde se escreve e se grava o firmware.
- **LVGL:** a "caixa de ferramentas" para desenhar a tela (botões, números, gráficos).
- **Splash:** a tela de abertura (o brasão) que aparece quando liga.
- **Prumo:** estar perfeitamente **na vertical** (em pé, sem cair para os lados).
- **Declividade / caimento:** a inclinação de uma superfície, medida em **%**.
- **dB (decibel):** unidade que mede o **volume** do som.
- **NR-15 / NBR 9050:** normas brasileiras (de ruído no trabalho e de acessibilidade)
  usadas aqui como **referência** prática.
- **Access Point (AP):** quando um aparelho **cria** uma rede WiFi, em vez de só se
  conectar a uma.
- **CSV:** arquivo de planilha simples (abre no Excel/Google Sheets).
- **Carta solar:** desenho do caminho que o sol faz no céu ao longo do dia.
- **Latitude / longitude:** as coordenadas que dizem **onde** no planeta fica um lugar.
- **Azimute:** a direção da bússola (Norte, Leste, Sul, Oeste) — é o que a opção
  "Bússola do cel" usa para descobrir para que lado uma parede está virada.

---

Feito com 💙 por **Murillo Vinícius** (@dantazzzz) e **Amanda Célia Aparecida da Silva**
— **Belas Artes (FEBASP)**. Licença **CC BY-NC 4.0**.
