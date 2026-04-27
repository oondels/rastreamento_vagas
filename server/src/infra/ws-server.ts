import http from "node:http";

import { WebSocketServer, WebSocket } from "ws";

import { AppConfig } from "../config/env";
import {
  FrontendBootstrapPayload,
  FrontendSpotSnapshotPayload,
  WebSocketMessage
} from "../contracts/frontend";
import { StateStore } from "../services/state-store";
import { Logger } from "./logger";

export class WsBridgeServer {
  private readonly clients = new Set<WebSocket>();
  private readonly httpServer: http.Server;
  private readonly wsServer: WebSocketServer;

  constructor(
    private readonly config: AppConfig,
    private readonly logger: Logger,
    private readonly stateStore: StateStore,
    private readonly mqttStatusProvider: () => boolean
  ) {
    this.httpServer = http.createServer(this.handleHttpRequest.bind(this));
    this.wsServer = new WebSocketServer({
      server: this.httpServer,
      path: this.config.wsPath,
      verifyClient: ({ origin }, done) => {
        if (this.isOriginAllowed(origin)) {
          done(true);
          return;
        }

        this.logger.warn("WebSocket connection rejected", { origin });
        done(false, 403, "Forbidden");
      }
    });
  }

  start(): Promise<void> {
    this.registerEventHandlers();

    return new Promise((resolve) => {
      this.httpServer.listen(this.config.port, () => {
        this.logger.info("Bridge server listening", {
          port: this.config.port,
          wsPath: this.config.wsPath
        });
        resolve();
      });
    });
  }

  close(): Promise<void> {
    for (const client of this.clients) {
      client.close();
    }

    return new Promise((resolve, reject) => {
      this.wsServer.close((wsError) => {
        if (wsError) {
          reject(wsError);
          return;
        }

        this.httpServer.close((httpError) => {
          if (httpError) {
            reject(httpError);
            return;
          }

          resolve();
        });
      });
    });
  }

  broadcastSnapshot(snapshot: FrontendSpotSnapshotPayload): void {
    this.broadcast({
      type: "parking:spots:snapshot",
      data: snapshot
    });
  }

  private registerEventHandlers(): void {
    this.wsServer.on("connection", (socket, request) => {
      this.clients.add(socket);
      this.logger.info("WebSocket client connected", {
        remoteAddress: request.socket.remoteAddress,
        clients: this.clients.size
      });

      this.sendBootstrap(socket);

      socket.on("close", () => {
        this.clients.delete(socket);
        this.logger.info("WebSocket client disconnected", {
          clients: this.clients.size
        });
      });

      socket.on("error", (error) => {
        this.logger.error("WebSocket client error", {
          error: error.message
        });
      });
    });
  }

  private sendBootstrap(socket: WebSocket): void {
    const payload: FrontendBootstrapPayload = {
      receivedAt: new Date().toISOString(),
      spots: this.stateStore.getAll()
    };

    this.sendMessage(socket, {
      type: "parking:spots:bootstrap",
      data: payload
    });
  }

  private broadcast(message: WebSocketMessage<unknown>): void {
    for (const client of this.clients) {
      this.sendMessage(client, message);
    }
  }

  private sendMessage(socket: WebSocket, message: WebSocketMessage<unknown>): void {
    if (socket.readyState !== WebSocket.OPEN) {
      return;
    }

    socket.send(JSON.stringify(message));
  }

  private handleHttpRequest(
    request: http.IncomingMessage,
    response: http.ServerResponse
  ): void {
    if (request.method === "GET" && request.url === "/health") {
      const body = JSON.stringify({
        status: "ok",
        mqttConnected: this.mqttStatusProvider(),
        timestamp: new Date().toISOString()
      });

      response.writeHead(200, { "Content-Type": "application/json" });
      response.end(body);
      return;
    }

    response.writeHead(404, { "Content-Type": "application/json" });
    response.end(JSON.stringify({ error: "Not found" }));
  }

  private isOriginAllowed(origin: string | undefined): boolean {
    if (this.config.wsAllowedOrigin === "*") {
      return true;
    }

    return origin === this.config.wsAllowedOrigin;
  }
}
