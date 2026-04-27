# Smart Parking

Monorepo do sistema Smart Parking com firmware ESP32, bridge MQTT -> WebSocket em Node.js e dashboard frontend em Vue 3 para visualizacao reativa das vagas.

![Build](https://img.shields.io/badge/build-passing-brightgreen)
![ESP32](https://img.shields.io/badge/device-ESP32-red)
![MQTT](https://img.shields.io/badge/protocol-MQTT-orange)
![WebSocket](https://img.shields.io/badge/realtime-WebSocket-0ea5e9)
![Vue](https://img.shields.io/badge/frontend-Vue_3-42b883)

## Visao Geral

O projeto foi organizado em tres camadas:

- `device/arduino`: firmware embarcado no ESP32
- `server/`: bridge backend que consome snapshots MQTT e retransmite eventos normalizados via WebSocket
- `app/`: dashboard cliente em Vue 3 que consome o bridge e mostra a ocupacao em tempo real

Fluxo fim a fim:

1. O ESP32 le sensores digitais das vagas.
2. O firmware publica snapshots JSON no broker MQTT.
3. O bridge Node.js assina `estacionamento/+/+/status`, normaliza o payload e mantem o ultimo estado em memoria.
4. O bridge envia `bootstrap` e `snapshot` para clientes WebSocket.
5. O frontend Vue atualiza o painel reativamente por `vagaId`.

![Arquitetura Geral](url-da-imagem)

## Estado Atual do Repositorio

### `device/arduino`

Implementado e funcional como firmware base do dispositivo IoT:

- configuracao carregada de `config.json` via LittleFS
- lista dinamica de vagas com limite `MAX_SPOTS`
- separacao de responsabilidades em modulos de configuracao, rede, MQTT, controlador e dominio
- publicacao MQTT centralizada no boot, em mudanca estavel e apos reconexao
- payload JSON consolidado para consumo do backend

Documentacao principal do firmware: [device/arduino/README.md](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/device/arduino/README.md:1)

### `server/`

Implementado como bridge MQTT -> WebSocket em Node.js + TypeScript:

- subscribe configuravel do broker MQTT
- validacao e normalizacao do contrato vindo do dispositivo
- store em memoria com o ultimo estado conhecido das vagas
- broadcast WebSocket para clientes conectados
- evento `parking:spots:bootstrap` na conexao
- evento `parking:spots:snapshot` a cada snapshot MQTT valido
- endpoint `GET /health`

Documentacao detalhada: [server/README.md](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/server/README.md:1)

### `app/`

Implementado como PoC frontend em Vue 3 + Vite + Tailwind CSS:

- conexao WebSocket nativa com `VITE_WS_URL`
- reconexao automatica com backoff simples
- estado reativo indexado por `vagaId`
- agrupamento visual por `siteId` e `deviceId`
- cards de vagas com status, identificador e timestamp da ultima atualizacao
- layout responsivo para mobile e desktop

Documentacao detalhada: [app/README.md](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/app/README.md:1)

## Estrutura do Repositorio

```text
.
|-- app/
|   |-- src/
|   |   |-- App.vue
|   |   `-- components/ParkingSpot.vue
|   |-- .env.example
|   |-- package.json
|   `-- README.md
|-- device/
|   `-- arduino/
|       |-- data/config.json.example
|       |-- include/
|       |-- src/
|       `-- platformio.ini
|-- server/
|   |-- src/
|   |-- .env.example
|   |-- package.json
|   `-- README.md
|-- .gitignore
`-- README.md
```

## Stack Tecnologica

| Camada | Stack | Finalidade |
|---|---|---|
| Device | C++, Arduino Framework, PlatformIO, ArduinoJson, LittleFS, PubSubClient | Leitura de sensores, controle embarcado e publicacao MQTT |
| Backend | Node.js, TypeScript, mqtt.js, ws | Bridge entre broker MQTT e clientes WebSocket |
| Frontend | Vue 3, Vite, Tailwind CSS | Dashboard reativo de vagas |
| Protocolo | MQTT + WebSocket + JSON | Integracao entre device, backend e frontend |

## Contratos de Integracao

### MQTT publicado pelo device

Topico:

```text
estacionamento/{siteId}/{deviceId}/status
```

Payload:

```json
{
  "siteId": "matriz",
  "deviceId": "esp32-setor-a",
  "timestampMs": 153021,
  "spots": [
    { "id": "A1", "occupied": true, "pin": 33 },
    { "id": "A2", "occupied": false, "pin": 25 }
  ]
}
```

### WebSocket publicado pelo bridge

Eventos:

- `parking:spots:bootstrap`
- `parking:spots:snapshot`

Payload normalizado por vaga:

```json
{
  "vagaId": "matriz:esp32-setor-a:A1",
  "localId": "A1",
  "siteId": "matriz",
  "deviceId": "esp32-setor-a",
  "ocupada": true,
  "lastUpdate": "2026-04-27T14:31:59.000Z",
  "deviceTimestampMs": 153021,
  "sourceTopic": "estacionamento/matriz/esp32-setor-a/status"
}
```

## Como Rodar Cada Camada

### 1. Firmware ESP32

```bash
cd device/arduino
cp data/config.json.example data/config.json
pio run
pio run --target uploadfs
pio run --target upload
pio device monitor -b 115200
```

### 2. Bridge backend

```bash
cd server
cp .env.example .env
npm install
npm run dev
```

### 3. Dashboard frontend

```bash
cd app
cp .env.example .env
npm install
npm run dev
```

Por padrao, o frontend espera o bridge em:

```text
ws://localhost:3001/ws
```

## Pre-requisitos

- Git
- Node.js 20+
- npm
- VS Code com PlatformIO ou PlatformIO Core CLI
- broker MQTT acessivel pelo backend e pelo dispositivo
- ESP32 compativel com `esp32dev`

## Observacoes Operacionais

- `device/arduino/data/config.json` nao deve ser versionado.
- o firmware exige `/config.json` no LittleFS; `config.json.example` nao e usado em runtime
- apos criar ou alterar `device/arduino/data/config.json`, execute `pio run -t uploadfs`
- `app/.env` e `server/.env` devem permanecer locais.
- `node_modules/`, `dist/`, `.pio/` e artefatos de build ja estao ignorados.
- O frontend depende do backend WebSocket para popular o dashboard; sem `bootstrap` ou `snapshot`, a UI permanece vazia por design.

## Documentacao por Modulo

- Firmware: [device/arduino/README.md](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/device/arduino/README.md:1)
- Backend: [server/README.md](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/server/README.md:1)
- Frontend: [app/README.md](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/app/README.md:1)

## Licenca

Defina a licenca oficial do projeto em um arquivo `LICENSE` na raiz. Ate isso acontecer, trate o repositorio como uso interno ou sob politica da equipe mantenedora.
