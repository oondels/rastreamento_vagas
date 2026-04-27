import mqtt, { IClientOptions, MqttClient } from "mqtt";

import { AppConfig } from "../config/env";
import { DeviceStatusMessage } from "../contracts/device";
import { FrontendSpotSnapshotPayload } from "../contracts/frontend";
import { Logger } from "./logger";

export interface MqttBridgeHandlers {
  onSnapshot: (snapshot: FrontendSpotSnapshotPayload) => void;
  normalize: (topic: string, payload: DeviceStatusMessage) => FrontendSpotSnapshotPayload;
}

export class MqttBridge {
  private client?: MqttClient;
  private connected = false;

  constructor(
    private readonly config: AppConfig,
    private readonly logger: Logger,
    private readonly handlers: MqttBridgeHandlers
  ) {}

  start(): void {
    const options: IClientOptions = {
      clientId: this.config.mqttClientId,
      username: this.config.mqttUsername,
      password: this.config.mqttPassword,
      reconnectPeriod: this.config.mqttReconnectPeriodMs
    };

    this.client = mqtt.connect(this.config.mqttUrl, options);
    this.registerEventHandlers(this.client);
  }

  isConnected(): boolean {
    return this.connected;
  }

  close(): Promise<void> {
    return new Promise((resolve) => {
      if (!this.client) {
        resolve();
        return;
      }

      this.client.end(false, {}, () => resolve());
    });
  }

  private registerEventHandlers(client: MqttClient): void {
    client.on("connect", () => {
      this.connected = true;
      this.logger.info("MQTT connected", {
        broker: this.config.mqttUrl,
        topic: this.config.mqttTopicPattern
      });
      this.subscribeToStatusTopic(client);
    });

    client.on("reconnect", () => {
      this.logger.warn("MQTT reconnecting");
    });

    client.on("close", () => {
      this.connected = false;
      this.logger.warn("MQTT connection closed");
    });

    client.on("offline", () => {
      this.connected = false;
      this.logger.warn("MQTT client offline");
    });

    client.on("error", (error) => {
      this.logger.error("MQTT error", { error: error.message });
    });

    client.on("message", (topic, message) => {
      this.handleMessage(topic, message);
    });
  }

  private subscribeToStatusTopic(client: MqttClient): void {
    client.subscribe(this.config.mqttTopicPattern, (error) => {
      if (error) {
        this.logger.error("MQTT subscribe failed", {
          topic: this.config.mqttTopicPattern,
          error: error.message
        });
        return;
      }

      this.logger.info("MQTT subscribed", { topic: this.config.mqttTopicPattern });
    });
  }

  private handleMessage(topic: string, message: Buffer): void {
    let parsedPayload: DeviceStatusMessage;

    try {
      parsedPayload = this.parseDeviceMessage(message);
    } catch (error) {
      const reason = error instanceof Error ? error.message : "Unknown parsing error";
      this.logger.error("MQTT message ignored", { topic, reason });
      return;
    }

    const snapshot = this.handlers.normalize(topic, parsedPayload);
    this.handlers.onSnapshot(snapshot);

    this.logger.info("MQTT message broadcasted", {
      topic,
      siteId: snapshot.siteId,
      deviceId: snapshot.deviceId,
      spots: snapshot.spots.length
    });
  }

  private parseDeviceMessage(message: Buffer): DeviceStatusMessage {
    const decodedPayload = JSON.parse(message.toString("utf-8")) as Partial<DeviceStatusMessage>;

    if (
      !decodedPayload ||
      typeof decodedPayload.siteId !== "string" ||
      typeof decodedPayload.deviceId !== "string" ||
      typeof decodedPayload.timestampMs !== "number" ||
      !Array.isArray(decodedPayload.spots)
    ) {
      throw new Error("Invalid device status payload structure");
    }

    const spots = decodedPayload.spots.map((spot, index) => {
      if (
        !spot ||
        typeof spot.id !== "string" ||
        typeof spot.occupied !== "boolean"
      ) {
        throw new Error(`Invalid spot payload at index ${index}`);
      }

      return {
        id: spot.id,
        occupied: spot.occupied,
        pin: typeof spot.pin === "number" ? spot.pin : undefined
      };
    });

    return {
      siteId: decodedPayload.siteId,
      deviceId: decodedPayload.deviceId,
      timestampMs: decodedPayload.timestampMs,
      spots
    };
  }
}
