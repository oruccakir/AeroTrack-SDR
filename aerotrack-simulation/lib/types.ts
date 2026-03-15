export interface RadarConfig {
  carrier_frequency_hz: number;
  bandwidth_hz: number;
  pulse_duration_s: number;
  prf_hz: number;
  num_pulses_per_cpi: number;
  num_samples_per_pulse: number;
  sampling_frequency_hz: number;
  snr_db: number;
  target_ip: string;
  target_port: number;
  udp_enabled: boolean;
}

export interface DerivedParams {
  wavelength_m: number;
  chirp_rate: number;
  pri_s: number;
  max_unambiguous_range_m: number;
  max_unambiguous_velocity_ms: number;
  range_resolution_m: number;
  velocity_resolution_ms: number;
  packet_size_bytes: number;
  data_rate_mbps: number;
}

export interface Target {
  range_m: number;
  velocity_kmh: number;
  rcs_m2: number;
  azimuth_deg: number;
  name: string;
}

export interface SimulationStatus {
  state: "IDLE" | "RUNNING" | "STOPPED";
  current_cpi: number;
  total_packets: number;
  elapsed_s: number;
  throughput_mbps: number;
  max_cpi_count: number;
}

export interface WaveformData {
  sample_count: number;
  i: number[];
  q: number[];
}

export interface RangeDopplerData {
  range_bins: number;
  doppler_bins: number;
  data: number[];
}

export interface TargetSnapshot {
  cpi_index: number;
  elapsed_s: number;
  targets: Target[];
}
