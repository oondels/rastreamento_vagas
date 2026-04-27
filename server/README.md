# Smart Parking Bridge Server

Servidor bridge em Node.js + TypeScript responsavel por conectar o broker MQTT do projeto Smart Parking a clientes WebSocket consumidos por um frontend, como a futura aplicacao Vue.js.

## Objetivo

Este servico atua como uma camada de integracao em tempo real entre:

- dispositivos ESP32 que publicam snapshots de estado via MQTT
- clientes frontend que precisam receber atualizacoes reativas por WebSocket

O bridge consome mensagens do broker, valida o payload publicado pelo firmware, normaliza cada vaga em um contrato consistente e faz broadcast para todos os clientes conectados.

## Arquitetura

Fluxo principal:

1. O ESP32 publica um snapshot JSON em `estacionamento/<siteId>/<deviceId>/status`
2. O bridge assina o topico configurado via `mqtt.js`
3. O payload e validado e convertido para um formato normalizado
4. O estado mais recente e mantido em memoria
5. O bridge envia as atualizacoes para os clientes WebSocket conectados via `ws`

### Responsabilidades por modulo

- `src/config/env.ts`
  Carrega e valida as variaveis de ambiente.
- `src/contracts/device.ts`
  Define o contrato de entrada vindo do dispositivo.
- `src/contracts/frontend.ts`
  Define o contrato de saida enviado ao frontend.
- `src/services/normalizer.ts`
  Normaliza snapshots do dispositivo para o formato publico do bridge.
- `src/services/state-store.ts`
  Mantem o ultimo estado conhecido das vagas em memoria.
- `src/infra/mqtt-bridge.ts`
  Encapsula conexao MQTT, subscribe, parsing e tratamento de eventos.
- `src/infra/ws-server.ts`
  Encapsula o servidor WebSocket, bootstrap inicial e broadcast.
- `src/infra/logger.ts`
  Centraliza logs operacionais.
- `src/index.ts`
  Faz a composicao da aplicacao e o lifecycle de inicializacao e encerramento.

## Contrato MQTT de entrada

O servidor segue o contrato real do firmware presente no repositorio e assina por padrao:

```text
estacionamento/+/+/status
```

Exemplo de payload publicado pelo ESP32:

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

## Contrato WebSocket de saida

O bridge envia mensagens JSON com a estrutura:

```json
{
  "type": "parking:spots:snapshot",
  "data": {}
}
```

### Evento `parking:spots:bootstrap`

Enviado automaticamente quando um cliente se conecta. Serve para sincronizar o frontend com o estado atual mantido em memoria.

Exemplo:

```json
{
  "type": "parking:spots:bootstrap",
  "data": {
    "receivedAt": "2026-04-27T14:32:10.000Z",
    "spots": [
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
    ]
  }
}
```

### Evento `parking:spots:snapshot`

Enviado a cada snapshot MQTT valido recebido.

Exemplo:

```json
{
  "type": "parking:spots:snapshot",
  "data": {
    "siteId": "matriz",
    "deviceId": "esp32-setor-a",
    "receivedAt": "2026-04-27T14:31:59.000Z",
    "spots": [
      {
        "vagaId": "matriz:esp32-setor-a:A1",
        "localId": "A1",
        "siteId": "matriz",
        "deviceId": "esp32-setor-a",
        "ocupada": true,
        "lastUpdate": "2026-04-27T14:31:59.000Z",
        "deviceTimestampMs": 153021,
        "sourceTopic": "estacionamento/matriz/esp32-setor-a/status"
      },
      {
        "vagaId": "matriz:esp32-setor-a:A2",
        "localId": "A2",
        "siteId": "matriz",
        "deviceId": "esp32-setor-a",
        "ocupada": false,
        "lastUpdate": "2026-04-27T14:31:59.000Z",
        "deviceTimestampMs": 153021,
        "sourceTopic": "estacionamento/matriz/esp32-setor-a/status"
      }
    ]
  }
}
```

## Configuracao

Crie um arquivo `.env` em `server/` a partir do exemplo:

```bash
cp .env.example .env
```

Variaveis suportadas:

| Variavel | Obrigatoria | Default | Descricao |
|---|---|---|---|
| `PORT` | Nao | `3001` | Porta HTTP usada pelo healthcheck e upgrade WebSocket |
| `WS_PATH` | Nao | `/ws` | Caminho do endpoint WebSocket |
| `WS_ALLOWED_ORIGIN` | Nao | `*` | Origem permitida no handshake WebSocket |
| `MQTT_URL` | Sim | - | URL do broker MQTT |
| `MQTT_USERNAME` | Nao | vazio | Usuario MQTT |
| `MQTT_PASSWORD` | Nao | vazio | Senha MQTT |
| `MQTT_CLIENT_ID` | Nao | `smart-parking-bridge` | Client ID usado pelo bridge |
| `MQTT_TOPIC_PATTERN` | Nao | `estacionamento/+/+/status` | Padrao de subscribe |
| `MQTT_RECONNECT_PERIOD_MS` | Nao | `5000` | Intervalo de reconexao MQTT em milissegundos |

Exemplo de `.env`:

```env
PORT=3001
WS_PATH=/ws
WS_ALLOWED_ORIGIN=http://localhost:5173
MQTT_URL=mqtt://192.168.1.10:1883
MQTT_USERNAME=usuario
MQTT_PASSWORD=senha
MQTT_CLIENT_ID=smart-parking-bridge
MQTT_TOPIC_PATTERN=estacionamento/+/+/status
MQTT_RECONNECT_PERIOD_MS=5000
```

## Como executar

### 1. Instalar dependencias

```bash
npm install
```

### 2. Rodar em desenvolvimento

```bash
npm run dev
```

### 3. Validar tipagem

```bash
npm run typecheck
```

### 4. Gerar build

```bash
npm run build
```

### 5. Executar build compilada

```bash
npm start
```

## Healthcheck

O servidor expõe um endpoint HTTP simples:

```text
GET /health
```

Resposta esperada:

```json
{
  "status": "ok",
  "mqttConnected": true,
  "timestamp": "2026-04-27T14:35:00.000Z"
}
```

## Exemplo de cliente WebSocket

Exemplo minimo em JavaScript para consumo futuro no frontend:

```js
const socket = new WebSocket("ws://localhost:3001/ws");

socket.addEventListener("message", (event) => {
  const message = JSON.parse(event.data);

  if (message.type === "parking:spots:bootstrap") {
    console.log("Estado inicial:", message.data.spots);
  }

  if (message.type === "parking:spots:snapshot") {
    console.log("Atualizacao recebida:", message.data.spots);
  }
});
```

## Logs e resiliencia

O bridge registra eventos operacionais relevantes:

- conexao e reconexao MQTT
- subscribe em topicos
- cliente MQTT offline ou conexao encerrada
- erros de parsing e payload invalido
- conexao e desconexao de clientes WebSocket
- broadcast de snapshots validos

Em caso de payload invalido, a mensagem e descartada com log de erro, sem derrubar o processo.

## Decisoes de design

- `ws` foi adotado no lugar de `socket.io` para manter o protocolo simples e sem camadas extras.
- O estado e mantido apenas em memoria, suficiente para sincronizacao inicial e broadcast reativo.
- `vagaId` e composto por `siteId:deviceId:spotId` para evitar colisao entre dispositivos.
- `lastUpdate` representa o horario de recebimento no servidor.
- `deviceTimestampMs` preserva o timestamp publicado pelo dispositivo para rastreabilidade.

## Melhorias futuras sugeridas

- autenticacao de clientes WebSocket
- rooms ou filtros por `siteId` e `deviceId`
- persistencia em banco ou cache distribuido
- suporte a TLS para MQTT e `wss`
- metricas e observabilidade com Prometheus ou OpenTelemetry
