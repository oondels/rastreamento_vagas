# Smart Parking Device Firmware

Firmware ESP32 do projeto Smart Parking, responsavel por ler sensores das vagas, aplicar debounce, carregar configuracao via LittleFS e publicar snapshots MQTT para o backend.

## Objetivo

Este modulo roda no dispositivo embarcado e faz a ponte entre o hardware do estacionamento e a camada de integracao em rede.

Responsabilidades principais:

- ler sensores digitais configurados por vaga
- carregar `config.json` do LittleFS
- conectar ao Wi-Fi e ao broker MQTT
- publicar snapshots completos de ocupacao
- manter o loop de operacao nao bloqueante

## Estrutura

```text
device/arduino/
|-- data/
|   `-- config.json.example
|-- include/
|-- src/
|-- test/
`-- platformio.ini
```

## Configuracao do LittleFS

O firmware exige um arquivo real em:

```text
/config.json
```

Esse arquivo precisa existir localmente em:

```text
device/arduino/data/config.json
```

O arquivo versionado no repositório e apenas o template:

```text
device/arduino/data/config.json.example
```

### Fluxo correto

1. Criar o arquivo real a partir do exemplo:

```bash
cd device/arduino
cp data/config.json.example data/config.json
```

2. Editar as credenciais e a lista de vagas em `data/config.json`

3. Gerar e gravar o filesystem:

```bash
pio run -t buildfs
pio run -t uploadfs
```

4. Subir o firmware:

```bash
pio run -t upload
```

5. Abrir o monitor serial:

```bash
pio device monitor -b 115200
```

## Importante

Executar apenas `pio run` ou apenas `pio run -t upload` **nao** envia o `config.json` para o device. O arquivo em `data/` so vai para o ESP32 quando `uploadfs` for executado.

Se voce rodar:

```bash
pio run -t buildfs
```

o output esperado deve listar `/config.json`. Se listar apenas `/config.json.example`, o device continuara falhando no boot porque o firmware abre exclusivamente `/config.json`.

## Exemplo de configuracao

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

## Troubleshooting

### Erro: `arquivo /config.json nao encontrado`

Causa mais comum:

- `device/arduino/data/config.json` nao foi criado
- `uploadfs` nao foi executado
- o LittleFS recebeu apenas `config.json.example`

Verificacao rapida:

```bash
cd device/arduino
ls data
pio run -t buildfs
```

Se o build do filesystem mostrar apenas:

```text
/config.json.example
```

o arquivo certo ainda nao esta sendo empacotado.

### Erro mesmo com `config.json` local

Nesse caso, geralmente o problema e um dos abaixo:

- `uploadfs` nao foi executado depois da criacao ou alteracao do arquivo
- a placa recebeu o firmware, mas nao recebeu a imagem do LittleFS atualizada
- o arquivo JSON esta invalido

## Comandos uteis

```bash
cd device/arduino
pio run
pio run -t buildfs
pio run -t uploadfs
pio run -t upload
pio device monitor -b 115200
```
