import dotenv from "dotenv";

dotenv.config();

export interface AppConfig {
  port: number;
  wsPath: string;
  wsAllowedOrigin: string;
  mqttUrl: string;
  mqttUsername?: string;
  mqttPassword?: string;
  mqttClientId: string;
  mqttTopicPattern: string;
  mqttReconnectPeriodMs: number;
}

function requireEnv(name: string): string {
  const value = process.env[name]?.trim();

  if (!value) {
    throw new Error(`Missing required environment variable: ${name}`);
  }

  return value;
}

function parseNumber(value: string | undefined, fallback: number, name: string): number {
  if (!value?.trim()) {
    return fallback;
  }

  const parsed = Number(value);
  if (!Number.isFinite(parsed) || parsed <= 0) {
    throw new Error(`Invalid numeric environment variable: ${name}`);
  }

  return parsed;
}

function normalizePath(pathValue: string | undefined): string {
  const rawValue = pathValue?.trim() || "/ws";

  if (rawValue === "/") {
    return rawValue;
  }

  return rawValue.startsWith("/") ? rawValue : `/${rawValue}`;
}

export function loadConfig(): AppConfig {
  return {
    port: parseNumber(process.env.PORT, 3001, "PORT"),
    wsPath: normalizePath(process.env.WS_PATH),
    wsAllowedOrigin: process.env.WS_ALLOWED_ORIGIN?.trim() || "*",
    mqttUrl: requireEnv("MQTT_URL"),
    mqttUsername: process.env.MQTT_USERNAME?.trim() || undefined,
    mqttPassword: process.env.MQTT_PASSWORD?.trim() || undefined,
    mqttClientId: process.env.MQTT_CLIENT_ID?.trim() || "smart-parking-bridge",
    mqttTopicPattern: process.env.MQTT_TOPIC_PATTERN?.trim() || "estacionamento/+/+/status",
    mqttReconnectPeriodMs: parseNumber(
      process.env.MQTT_RECONNECT_PERIOD_MS,
      5000,
      "MQTT_RECONNECT_PERIOD_MS"
    )
  };
}
