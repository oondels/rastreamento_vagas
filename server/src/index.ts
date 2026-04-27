import { loadConfig } from "./config/env";
import { Logger } from "./infra/logger";
import { MqttBridge } from "./infra/mqtt-bridge";
import { WsBridgeServer } from "./infra/ws-server";
import { normalizeDeviceStatus } from "./services/normalizer";
import { StateStore } from "./services/state-store";

async function bootstrap(): Promise<void> {
  const config = loadConfig();
  const logger = new Logger();
  const stateStore = new StateStore();

  let wsServer: WsBridgeServer | undefined;

  const mqttBridge = new MqttBridge(config, logger, {
    normalize: (topic, payload) => normalizeDeviceStatus({ topic, payload }),
    onSnapshot: (snapshot) => {
      stateStore.upsertMany(snapshot.spots);
      wsServer?.broadcastSnapshot(snapshot);
    }
  });

  wsServer = new WsBridgeServer(config, logger, stateStore, () => mqttBridge.isConnected());

  await wsServer.start();
  mqttBridge.start();

  const shutdown = async (signal: NodeJS.Signals): Promise<void> => {
    logger.info("Shutdown signal received", { signal });

    await Promise.allSettled([mqttBridge.close(), wsServer?.close() ?? Promise.resolve()]);
    process.exit(0);
  };

  process.on("SIGINT", () => {
    void shutdown("SIGINT");
  });

  process.on("SIGTERM", () => {
    void shutdown("SIGTERM");
  });
}

bootstrap().catch((error) => {
  const logger = new Logger();
  logger.error("Application failed to start", {
    error: error instanceof Error ? error.message : "Unknown error"
  });
  process.exit(1);
});
