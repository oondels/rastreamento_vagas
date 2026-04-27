# Smart Parking App

Dashboard frontend em Vue 3 para visualizacao reativa da ocupacao de vagas publicada pelo bridge WebSocket do projeto Smart Parking.

![Vue](https://img.shields.io/badge/Vue-3-42b883)
![Vite](https://img.shields.io/badge/Vite-5-646cff)
![Tailwind](https://img.shields.io/badge/Tailwind-3-38bdf8)
![WebSocket](https://img.shields.io/badge/realtime-WebSocket-0ea5e9)

![Preview do Dashboard](url-da-imagem)

## Objetivo

Esta aplicacao consome o bridge backend em `server/` por WebSocket e exibe o estado atual das vagas de estacionamento em tempo real. O foco atual e uma PoC enxuta, responsiva e operacionalmente clara.

## Funcionalidades

- conexao WebSocket nativa sem biblioteca cliente adicional
- consumo dos eventos `parking:spots:bootstrap` e `parking:spots:snapshot`
- estado reativo indexado por `vagaId`
- inclusao dinamica de novas vagas recebidas do servidor
- agrupamento visual por `siteId` e `deviceId`
- cards com identificador da vaga, status e timestamp da ultima atualizacao
- indicador de conexao com estados `Conectado`, `Reconectando` e `Desconectado`
- reconexao automatica com backoff simples
- layout responsivo para mobile e desktop

## Stack

| Camada | Tecnologia |
|---|---|
| Framework | Vue 3 |
| Bundler | Vite |
| Estilo | Tailwind CSS |
| Tempo real | WebSocket nativo do navegador |

## Estrutura

```text
app/
|-- src/
|   |-- components/
|   |   `-- ParkingSpot.vue
|   |-- App.vue
|   |-- main.js
|   `-- style.css
|-- .env.example
|-- index.html
|-- package.json
|-- postcss.config.js
|-- tailwind.config.js
|-- vite.config.js
`-- README.md
```

## Contrato de Entrada

O frontend espera mensagens do bridge com o formato:

```json
{
  "type": "parking:spots:snapshot",
  "data": {}
}
```

### Evento `parking:spots:bootstrap`

Usado para sincronizacao inicial ou ressincronizacao apos reconexao:

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

Usado para atualizar ou adicionar vagas recebidas em tempo real:

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
      }
    ]
  }
}
```

## Configuracao de Ambiente

Crie o arquivo local a partir do exemplo:

```bash
cp .env.example .env
```

Variaveis suportadas:

| Variavel | Obrigatoria | Default | Descricao |
|---|---|---|---|
| `VITE_WS_URL` | Nao | `ws://localhost:3001/ws` | URL completa do bridge WebSocket |

Exemplo:

```env
VITE_WS_URL=ws://localhost:3001/ws
```

## Instalacao

```bash
cd app
npm install
```

## Execucao em Desenvolvimento

```bash
npm run dev
```

O Vite normalmente sobe em `http://localhost:5173`.

## Build de Producao

```bash
npm run build
```

Para validar o bundle localmente:

```bash
npm run preview
```

## Como a Aplicacao Funciona

### Estado reativo

O estado principal fica em `src/App.vue`:

- `spotsById`: mapa reativo indexado por `vagaId`
- `connectionStatus`: estado atual da conexao WebSocket
- `lastMessageAt`: horario do ultimo payload recebido

### Reconciliacao dos dados

- `bootstrap` substitui o mapa inteiro de vagas
- `snapshot` faz merge por `vagaId`
- se uma vaga nova chegar pelo socket, ela e adicionada automaticamente ao painel

### Componentizacao

- `App.vue`
  Orquestra conexao, agrupamento, resumo e layout principal
- `ParkingSpot.vue`
  Renderiza o card atomico de uma vaga

## UI e UX

- vagas ocupadas aparecem em vermelho
- vagas livres aparecem em verde
- a interface exibe agrupamento por `siteId` e `deviceId`
- o resumo superior mostra total, ocupadas, livres e ultima sincronizacao
- o estado do WebSocket e sempre visivel

## Integracao com o Backend

Antes de subir o frontend, o bridge backend deve estar rodando em `server/`.

Fluxo minimo:

```bash
cd server
cp .env.example .env
npm install
npm run dev
```

Depois:

```bash
cd app
cp .env.example .env
npm install
npm run dev
```

## Troubleshooting

### Dashboard abre, mas nao mostra vagas

Possiveis causas:

- o backend nao esta rodando
- o bridge nao recebeu nenhum `bootstrap` ou `snapshot`
- `VITE_WS_URL` esta apontando para a URL errada

Verificacoes:

- confirmar se o bridge responde `GET /health`
- confirmar no console do navegador se o WebSocket conectou
- validar o valor de `VITE_WS_URL`

### Status fica em desconectado ou reconectando

Possiveis causas:

- bridge offline
- porta errada
- path WS incorreto
- origem bloqueada pelo backend

Verificacoes:

- conferir `WS_ALLOWED_ORIGIN` no backend
- validar se o bridge esta em `ws://localhost:3001/ws`
- inspecionar logs do `server/`

### Build falha

Possiveis causas:

- dependencias nao instaladas
- versao antiga de Node.js

Verificacoes:

- rodar `npm install`
- usar Node.js 20+ para manter alinhamento com o monorepo

## Scripts Disponiveis

| Script | Descricao |
|---|---|
| `npm run dev` | Sobe o Vite em modo desenvolvimento |
| `npm run build` | Gera o bundle de producao |
| `npm run preview` | Serve localmente o bundle gerado |

## Arquivos Principais

- [src/App.vue](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/app/src/App.vue:1)
- [src/components/ParkingSpot.vue](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/app/src/components/ParkingSpot.vue:1)
- [src/style.css](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/app/src/style.css:1)
- [package.json](/home/oendel/Documentos/PlatformIO/Projects/mapeamento_vagas/app/package.json:1)

## Estado Atual

O frontend esta no estagio de PoC funcional. Ele ja cobre visualizacao em tempo real e reconexao automatica, mas ainda nao inclui:

- autenticacao
- filtros avancados
- persistencia local
- modo administrativo
- testes automatizados
