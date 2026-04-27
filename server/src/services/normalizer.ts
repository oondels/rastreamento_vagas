import { DeviceStatusMessage } from "../contracts/device";
import { FrontendSpotSnapshotPayload, FrontendSpotStatus } from "../contracts/frontend";

export interface NormalizationInput {
  topic: string;
  payload: DeviceStatusMessage;
  receivedAt?: Date;
}

export function normalizeDeviceStatus({
  topic,
  payload,
  receivedAt = new Date()
}: NormalizationInput): FrontendSpotSnapshotPayload {
  const receivedAtIso = receivedAt.toISOString();
  const spots = payload.spots.map((spot) =>
    mapSpotStatus({
      topic,
      payload,
      receivedAtIso,
      spotId: spot.id,
      occupied: spot.occupied
    })
  );

  return {
    siteId: payload.siteId,
    deviceId: payload.deviceId,
    receivedAt: receivedAtIso,
    spots
  };
}

interface SpotMapperParams {
  topic: string;
  payload: DeviceStatusMessage;
  receivedAtIso: string;
  spotId: string;
  occupied: boolean;
}

function mapSpotStatus({
  topic,
  payload,
  receivedAtIso,
  spotId,
  occupied
}: SpotMapperParams): FrontendSpotStatus {
  return {
    vagaId: `${payload.siteId}:${payload.deviceId}:${spotId}`,
    localId: spotId,
    siteId: payload.siteId,
    deviceId: payload.deviceId,
    ocupada: occupied,
    lastUpdate: receivedAtIso,
    deviceTimestampMs: payload.timestampMs,
    sourceTopic: topic
  };
}
