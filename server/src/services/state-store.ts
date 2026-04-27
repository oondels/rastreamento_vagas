import { FrontendSpotStatus } from "../contracts/frontend";

export class StateStore {
  private readonly spotsById = new Map<string, FrontendSpotStatus>();

  upsertMany(spots: FrontendSpotStatus[]): void {
    for (const spot of spots) {
      this.spotsById.set(spot.vagaId, spot);
    }
  }

  getAll(): FrontendSpotStatus[] {
    return Array.from(this.spotsById.values()).sort((left, right) =>
      left.vagaId.localeCompare(right.vagaId)
    );
  }
}
