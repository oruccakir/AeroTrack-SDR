import type {
  RadarConfig,
  DerivedParams,
  Target,
  SimulationStatus,
  WaveformData,
  RangeDopplerData,
  TargetSnapshot,
} from "./types";

async function fetchJSON<T>(url: string, init?: RequestInit): Promise<T> {
  const res = await fetch(url, init);
  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: res.statusText }));
    throw new Error(err.error || res.statusText);
  }
  return res.json() as Promise<T>;
}

export const api = {
  getHealth: () => fetchJSON<{ status: string }>("/api/health"),

  getConfig: () => fetchJSON<RadarConfig>("/api/config"),

  putConfig: (config: Partial<RadarConfig>) =>
    fetchJSON<{ status: string }>("/api/config", {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(config),
    }),

  getDerived: () => fetchJSON<DerivedParams>("/api/derived"),

  startSimulation: (cpiCount?: number) =>
    fetchJSON<{ status: string }>("/api/simulation/start", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(cpiCount ? { cpi_count: cpiCount } : {}),
    }),

  stopSimulation: () =>
    fetchJSON<{ status: string }>("/api/simulation/stop", {
      method: "POST",
    }),

  getStatus: () => fetchJSON<SimulationStatus>("/api/simulation/status"),

  getTargets: () => fetchJSON<Target[]>("/api/targets"),

  addTarget: (target: Partial<Target>) =>
    fetchJSON<{ status: string; index: number }>("/api/targets", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(target),
    }),

  deleteTarget: (index: number) =>
    fetchJSON<{ status: string }>(`/api/targets/${index}`, {
      method: "DELETE",
    }),

  getWaveform: () => fetchJSON<WaveformData>("/api/data/waveform"),

  getRangeDoppler: () =>
    fetchJSON<RangeDopplerData>("/api/data/range-doppler"),

  getTargetHistory: () =>
    fetchJSON<TargetSnapshot[]>("/api/data/target-history"),
};
