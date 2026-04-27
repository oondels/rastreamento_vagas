# Smart Parking ESP32

Dispositivo IoT baseado em ESP32 para monitorar a ocupacao de vagas de estacionamento e publicar snapshots de estado em tempo real via MQTT.

![Build](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-blue)
![MQTT](https://img.shields.io/badge/protocol-MQTT-orange)
![ESP32](https://img.shields.io/badge/hardware-ESP32-red)

![Diagrama de Blocos](url-da-imagem)
![Topologia MQTT](url-da-imagem)

## Sumario

- [Visao Geral](#visao-geral)
- [Funcionalidades](#funcionalidades)
- [Estrutura do Repositorio](#estrutura-do-repositorio)
- [Hardware Necessario](#hardware-necessario)
- [Stack Tecnologica](#stack-tecnologica)
- [Pre-requisitos](#pre-requisitos)
- [Configuracao do Dispositivo Passo a Passo](#configuracao-do-dispositivo-passo-a-passo)
- [Arquitetura MQTT](#arquitetura-mqtt)
- [Instalacao e Uso](#instalacao-e-uso)
- [Troubleshooting](#troubleshooting)
- [Contribuicao](#contribuicao)
- [Licenca](#licenca)

## Visao Geral

O projeto `Smart Parking ESP32` foi estruturado para operar como a camada de borda de um sistema de estacionamento inteligente. O firmware roda em um ESP32, le os sensores associados a vagas configuradas dinamicamente, aplica debounce por software, monta um snapshot JSON com o estado consolidado e o publica em um broker MQTT.

O repositorio foi organizado como monorepo com tres dominos principais:

- `device/arduino`: firmware embarcado do ESP32
- `server/`: backend futuro responsavel pela ponte broker -> aplicacao
- `app/`: frontend futuro responsavel pela visualizacao do mapa de vagas

## Funcionalidades

- **Deteccao em tempo real** da ocupacao das vagas com leitura continua no loop principal.
- **Configuracao dinamica de vagas** via `config.json`, sem quantidade fixa hardcoded no firmware.
- **Separacao de responsabilidades** em modulos de configuracao, dominio, rede, MQTT e orquestracao.
- **Publicacao MQTT centralizada** com snapshot completo do dispositivo.
- **Boot com publicacao inicial** para sincronizar o estado logo apos a inicializacao.
- **Republicacao em mudanca estavel** de qualquer vaga apos debounce temporal.
- **Reconexao nao bloqueante** de Wi-Fi e MQTT usando `millis()`.
- **Availability MQTT com LWT** para indicar `online` e `offline`.
- **Persistencia de configuracao em LittleFS** no ESP32.
- **Payloads JSON padronizados** para integracao simples com backend e frontend.
- **Limite de seguranca por `MAX_SPOTS`** para previsibilidade de memoria.
- **Protecao basica de credenciais** com `config.json` fora do versionamento.

## Estrutura do Repositorio

```text
.
|-- app/
|   `-- .gitkeep
|-- device/
|   `-- arduino/
|       |-- data/
|       |   `-- config.json.example
|       |-- include/
|       |   |-- AppConfig.h
|       |   |-- ConfigManager.h
|       |   |-- MqttManager.h
|       |   |-- NetworkManager.h
|       |   |-- ParkingController.h
|       |   |-- ParkingSpot.h
|       |   `-- ParkingSpotManager.h
|       |-- lib/
|       |-- src/
|       |   |-- ConfigManager.cpp
|       |   |-- MqttManager.cpp
|       |   |-- NetworkManager.cpp
|       |   |-- ParkingController.cpp
|       |   |-- ParkingSpot.cpp
|       |   |-- ParkingSpotManager.cpp
|       |   `-- main.cpp
|       |-- test/
|       `-- platformio.ini
|-- server/
|   `-- .gitkeep
|-- .gitignore
`-- README.md
```

## Hardware Necessario

### Componentes

- **1x ESP32 DevKit** ou placa compativel com `board = esp32dev`
- **1 sensor por vaga**
  - Opcao 1: sensor infravermelho digital
  - Opcao 2: sensor ultrassonico com condicionamento de saida
  - Opcao 3: qualquer sensor que entregue leitura digital estavel em GPIO
- **Fonte de alimentacao regulada**
  - `5V` para alimentacao da placa, conforme seu board
  - `3.3V` para sensores quando exigido pelo modulo
- **Protoboard** para prototipagem ou **PCB** para instalacao definitiva
- **Cabos jumper** macho-macho / macho-femea conforme a montagem
- **Resistores e componentes de condicionamento** caso o sensor nao forneca nivel logico adequado
- **Caixa de protecao** para instalacao em ambiente semiaberto ou externo

### Observacoes de engenharia eletrica

- O ESP32 opera em **3.3V logicos**. Nao conecte sinais digitais de `5V` diretamente aos GPIOs sem adaptacao.
- Em sensores com saida aberta, ruidosa ou flutuante, adote resistores de pull-up ou pull-down conforme o circuito.
- Sensores ultrassonicos nem sempre fornecem diretamente um nivel digital simples para esse firmware. Se usar HC-SR04, por exemplo, sera necessario um circuito e uma camada de software diferente da implementada hoje.
- O firmware atual assume leitura por `digitalRead()`, portanto o hardware deve expor um **sinal digital binario por vaga**.

### Esquema de pinagem

O mapeamento de pinos nao fica fixo no codigo. Cada vaga e definida no arquivo `device/arduino/data/config.json` por meio do campo `pin`.

Exemplo de pinagem:

| Vaga | GPIO ESP32 | Sensor | Nivel de ocupacao |
|---|---:|---|---|
| `A1` | `33` | Infravermelho | `HIGH = ocupada` |
| `A2` | `25` | Infravermelho | `HIGH = ocupada` |
| `A3` | `26` | Infravermelho | `HIGH = ocupada` |
| `A4` | `27` | Infravermelho | `HIGH = ocupada` |

Exemplo da secao `spots`:

```json
"spots": [
  { "id": "A1", "pin": 33, "occupiedOnHigh": true },
  { "id": "A2", "pin": 25, "occupiedOnHigh": true },
  { "id": "A3", "pin": 26, "occupiedOnHigh": true },
  { "id": "A4", "pin": 27, "occupiedOnHigh": true }
]
```

### Recomendacoes de pinagem no ESP32

- Prefira GPIOs estaveis e adequados para entrada digital.
- Evite GPIOs com funcao de boot strap sem conhecer o impacto no startup da placa.
- Valide em bancada se o sensor mantem o nivel logico correto no boot e durante ruido eletromagnetico.
- Documente localmente a relacao `vaga -> sensor -> cabo -> GPIO` antes de instalacao em campo.

## Stack Tecnologica

| Camada | Tecnologia | Finalidade |
|---|---|---|
| Firmware | **C++** com **Arduino Framework** | Controle de hardware e logica embarcada |
| Build | **PlatformIO** | Build, flash, monitor serial e upload de filesystem |
| Conectividade | **Wi-Fi** | Acesso do ESP32 a rede local |
| Mensageria | **MQTT** | Transporte de estado do dispositivo |
| Serializacao | **ArduinoJson** | Montagem e leitura de payloads JSON |
| Armazenamento local | **LittleFS** | Persistencia de `config.json` no flash do ESP32 |

## Pre-requisitos

### Software

- **Git**
- **Visual Studio Code** com extensao **PlatformIO IDE**
- Alternativamente, **PlatformIO Core CLI**
- Opcionalmente, **Arduino IDE**
  - Recomendado apenas para referencia; o fluxo principal deste projeto usa PlatformIO
- Um **broker MQTT** acessivel pela rede do dispositivo
  - Exemplo: Mosquitto, EMQX, HiveMQ

### Ambiente de desenvolvimento recomendado

- Sistema operacional Linux, macOS ou Windows
- Porta USB funcional para gravacao da placa
- Permissao de acesso a serial USB
- Rede Wi-Fi 2.4 GHz acessivel pelo ESP32

### Dependencias do firmware

Configuradas em `device/arduino/platformio.ini`:

- `knolleary/PubSubClient`
- `bblanchon/ArduinoJson`

O projeto tambem utiliza as bibliotecas do ecossistema ESP32/Arduino:

- `WiFi`
- `LittleFS`

## Configuracao do Dispositivo Passo a Passo

### 1. Clonar o repositorio

```bash
git clone <url-do-repositorio>
cd mapeamento_vagas
```

Se estiver usando VS Code, abra a raiz do projeto:

```bash
code .
```

### 2. Entrar no diretorio do firmware

Todos os comandos de build e flash devem ser executados dentro de `device/arduino`:

```bash
cd device/arduino
```

### 3. Criar o arquivo real de configuracao

Copie o template versionado:

```bash
cp data/config.json.example data/config.json
```

O arquivo `data/config.json` sera enviado para o LittleFS do ESP32 e **nao deve ser versionado**.

### 4. Editar o `config.json`

Exemplo completo:

```json
{
  "wifi": {
    "ssid": "SEU_WIFI",
    "password": "SUA_SENHA"
  },
  "mqtt": {
    "host": "192.168.1.10",
    "port": 1883,
    "username": "usuario",
    "password": "senha",
    "clientId": "esp32-setor-a",
    "publishRetained": true
  },
  "device": {
    "siteId": "matriz",
    "deviceId": "esp32-setor-a"
  },
  "timing": {
    "sensorDebounceMs": 120,
    "wifiReconnectIntervalMs": 10000,
    "mqttReconnectIntervalMs": 5000,
    "publishIntervalMs": 0
  },
  "spots": [
    { "id": "A1", "pin": 33, "occupiedOnHigh": true },
    { "id": "A2", "pin": 25, "occupiedOnHigh": true }
  ]
}
```

### 5. Entender cada campo do `config.json`

#### Bloco `wifi`

| Campo | Tipo | Obrigatorio | Descricao |
|---|---|---|---|
| `ssid` | `string` | Sim | Nome da rede Wi-Fi 2.4 GHz |
| `password` | `string` | Sim | Senha da rede Wi-Fi |

#### Bloco `mqtt`

| Campo | Tipo | Obrigatorio | Descricao |
|---|---|---|---|
| `host` | `string` | Sim | IP ou hostname do broker MQTT |
| `port` | `number` | Sim | Porta do broker, normalmente `1883` |
| `username` | `string` | Sim, se o broker exigir | Usuario MQTT |
| `password` | `string` | Sim, se o broker exigir | Senha MQTT |
| `clientId` | `string` | Sim | Identificador da sessao MQTT do dispositivo |
| `publishRetained` | `boolean` | Sim | Define se o payload de status sera publicado como retained |

#### Bloco `device`

| Campo | Tipo | Obrigatorio | Descricao |
|---|---|---|---|
| `siteId` | `string` | Sim | Identificador logico do local, ex: `matriz` |
| `deviceId` | `string` | Sim | Identificador unico do dispositivo, ex: `esp32-setor-a` |

#### Bloco `timing`

| Campo | Tipo | Obrigatorio | Descricao |
|---|---|---|---|
| `sensorDebounceMs` | `number` | Sim | Tempo minimo para aceitar uma mudanca estavel no sensor |
| `wifiReconnectIntervalMs` | `number` | Sim | Intervalo entre tentativas de reconexao Wi-Fi |
| `mqttReconnectIntervalMs` | `number` | Sim | Intervalo entre tentativas de reconexao MQTT |
| `publishIntervalMs` | `number` | Sim | `0` para publicar apenas no boot e em mudancas; acima de `0` habilita publicacao periodica |

#### Bloco `spots`

| Campo | Tipo | Obrigatorio | Descricao |
|---|---|---|---|
| `id` | `string` | Sim | Identificador logico da vaga |
| `pin` | `number` | Sim | GPIO do ESP32 conectado ao sensor da vaga |
| `occupiedOnHigh` | `boolean` | Sim | `true` se `HIGH = ocupada`; `false` se `LOW = ocupada` |

### 6. Regras de validacao do firmware

Na inicializacao, o firmware valida:

- Existencia de `/config.json` no LittleFS
- Presenca das secoes `wifi`, `mqtt`, `device`, `timing` e `spots`
- `spots` nao vazio
- Quantidade de vagas menor ou igual a `MAX_SPOTS`
- `id` obrigatorio e unico por vaga
- `pin` obrigatorio
- `occupiedOnHigh` obrigatorio

Se houver erro, o dispositivo aborta a inicializacao logica e reporta o problema na serial.

### 7. Proteger credenciais com `.gitignore`

O repositorio ja inclui regras para evitar commit de credenciais:

```gitignore
/.pio/
/.vscode/
device/arduino/.pio/
device/arduino/.vscode/
device/arduino/data/config.json
!device/arduino/data/config.json.example
```

Use sempre o fluxo:

- versionar `data/config.json.example`
- manter `data/config.json` apenas localmente
- jamais comitar SSID, senha Wi-Fi, usuario MQTT ou senha MQTT reais

### 8. Gerar e enviar o Filesystem Image para o ESP32

Com o `config.json` criado em `device/arduino/data/config.json`, execute:

```bash
pio run --target buildfs
pio run --target uploadfs
```

Esses comandos:

- empacotam o conteudo da pasta `data/`
- geram a imagem LittleFS
- gravam a imagem no flash do ESP32

### 9. Confirmar a configuracao no boot

Abra o monitor serial:

```bash
pio device monitor -b 115200
```

Mensagens esperadas:

- inicializacao do controlador
- montagem do LittleFS
- configuracao carregada
- tentativa de conexao Wi-Fi
- tentativa de conexao MQTT
- publicacao inicial de status

## Arquitetura MQTT

### Topicos utilizados

O firmware atual **publica** mensagens de status e disponibilidade. Nao ha fluxo de subscribe implementado neste momento.

| Direcao | Topico | QoS | Retained | Descricao |
|---|---|---:|---|---|
| Publish | `estacionamento/{siteId}/{deviceId}/status` | `0` | Configuravel por `publishRetained` | Snapshot consolidado das vagas |
| Publish | `estacionamento/{siteId}/{deviceId}/availability` | `0` | `true` | Estado de disponibilidade do dispositivo |
| LWT | `estacionamento/{siteId}/{deviceId}/availability` | `0` | `true` | Publica `offline` se a conexao cair abruptamente |
| Subscribe | Nenhum | - | - | Nao implementado nesta versao |

### Politica de publicacao

- Publica um snapshot completo no boot, assim que houver conexao MQTT.
- Publica um snapshot completo quando qualquer vaga tiver mudanca estavel.
- Publica novamente apos reconexao com o broker.
- Pode publicar periodicamente se `publishIntervalMs > 0`.

### Exemplo de payload JSON

```json
{
  "siteId": "matriz",
  "deviceId": "esp32-setor-a",
  "timestampMs": 1234567,
  "spots": [
    { "id": "A1", "occupied": true, "pin": 33 },
    { "id": "A2", "occupied": false, "pin": 25 }
  ]
}
```

### Semantica do payload

| Campo | Tipo | Descricao |
|---|---|---|
| `siteId` | `string` | Agrupador logico do local |
| `deviceId` | `string` | Identificador do dispositivo no local |
| `timestampMs` | `number` | Timestamp relativo em `millis()` desde o boot |
| `spots[].id` | `string` | Identificador da vaga |
| `spots[].occupied` | `boolean` | Estado estavel da vaga |
| `spots[].pin` | `number` | GPIO associado a vaga |

### Exemplo de disponibilidade

Dispositivo online:

```text
Topic: estacionamento/matriz/esp32-setor-a/availability
Payload: online
```

Dispositivo offline por LWT:

```text
Topic: estacionamento/matriz/esp32-setor-a/availability
Payload: offline
```

## Instalacao e Uso

### Compilar o firmware

```bash
cd device/arduino
pio run
```

### Fazer upload do firmware

```bash
cd device/arduino
pio run --target upload
```

### Fazer upload do filesystem

```bash
cd device/arduino
pio run --target uploadfs
```

### Monitorar logs serial

```bash
cd device/arduino
pio device monitor -b 115200
```

### Fluxo completo recomendado

```bash
cd device/arduino
cp data/config.json.example data/config.json
# editar data/config.json
pio run
pio run --target uploadfs
pio run --target upload
pio device monitor -b 115200
```

## Troubleshooting

### 1. ESP32 nao conecta no Wi-Fi

Possiveis causas:

- `ssid` ou `password` incorretos
- rede operando somente em `5 GHz`
- sinal fraco no local da instalacao
- filtragem MAC ou restricao de DHCP

Acoes recomendadas:

- validar SSID e senha no `config.json`
- testar a rede com outro dispositivo 2.4 GHz
- aproximar a placa do access point
- observar logs no monitor serial

### 2. ESP32 nao conecta no broker MQTT

Possiveis causas:

- `host` ou `port` incorretos
- broker fora do ar
- firewall ou ACL bloqueando a conexao
- `username` e `password` invalidos
- `clientId` duplicado com outro dispositivo ativo

Acoes recomendadas:

- testar conectividade ao broker na mesma rede
- verificar se a porta `1883` esta acessivel
- confirmar credenciais
- trocar o `clientId` por um valor unico
- consultar o log do broker

### 3. O status nao aparece no backend

Possiveis causas:

- topico consumido incorretamente no servidor
- broker sem retained e app conectando depois
- falha de serializacao ou perda de conexao MQTT

Acoes recomendadas:

- inspecionar o topico `estacionamento/{siteId}/{deviceId}/status`
- usar um cliente MQTT para confirmar mensagens em tempo real
- verificar se `publishRetained` esta adequado ao seu fluxo

### 4. Sensor apresenta ruido ou falso positivo

Possiveis causas:

- alimentacao instavel
- cabo longo sem blindagem
- sensor analogico adaptado de forma inadequada
- `sensorDebounceMs` muito baixo

Acoes recomendadas:

- aumentar `sensorDebounceMs`
- revisar aterramento e fonte
- usar filtro de hardware quando necessario
- validar o nivel logico com multimetro ou osciloscopio

### 5. O firmware inicia, mas aborta logo no boot

Possiveis causas:

- `config.json` ausente no LittleFS
- JSON invalido
- `spots` vazio
- IDs duplicados
- quantidade de vagas acima de `MAX_SPOTS`

Acoes recomendadas:

- regenerar e reenviar o filesystem com `uploadfs`
- validar a sintaxe JSON
- revisar o bloco `spots`

### 6. O upload do filesystem falha

Possiveis causas:

- placa errada selecionada
- porta serial incorreta
- dispositivo sem permissao USB
- ESP32 em modo de boot inadequado

Acoes recomendadas:

- confirmar `board = esp32dev` em `platformio.ini`
- selecionar a porta correta no PlatformIO
- desconectar outros dispositivos seriais
- pressionar `BOOT` durante o upload se a placa exigir

## Contribuicao

Contribuicoes sao bem-vindas, principalmente nas areas de:

- robustez de hardware e sensores
- testes embarcados
- backend de integracao MQTT
- visualizacao frontend do mapa de vagas
- observabilidade e operacao do sistema

Padrao minimo recomendado para contribuicoes:

1. Crie uma branch a partir de `main`.
2. Mantenha commits pequenos e semanticamente claros.
3. Atualize documentacao e exemplos de configuracao quando houver mudanca de comportamento.
4. Nunca inclua credenciais reais em commits.
5. Sempre valide `pio run` antes de abrir PR.

## Licenca

Este projeto pode ser distribuido sob a licenca **MIT** ou outra licenca definida pela equipe mantenedora.

Adicione o arquivo `LICENSE` na raiz do repositorio para formalizar a politica de distribuicao.
