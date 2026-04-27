export interface DeviceSpotStatus {
  id: string;
  occupied: boolean;
  pin?: number;
}

export interface DeviceStatusMessage {
  siteId: string;
  deviceId: string;
  timestampMs: number;
  spots: DeviceSpotStatus[];
}
