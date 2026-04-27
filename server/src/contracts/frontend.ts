export interface FrontendSpotStatus {
  vagaId: string;
  localId: string;
  siteId: string;
  deviceId: string;
  ocupada: boolean;
  lastUpdate: string;
  deviceTimestampMs: number;
  sourceTopic: string;
}

export interface FrontendSpotSnapshotPayload {
  siteId: string;
  deviceId: string;
  receivedAt: string;
  spots: FrontendSpotStatus[];
}

export interface FrontendBootstrapPayload {
  receivedAt: string;
  spots: FrontendSpotStatus[];
}

export interface WebSocketMessage<T> {
  type: string;
  data: T;
}
