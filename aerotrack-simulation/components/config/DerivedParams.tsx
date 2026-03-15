import type { DerivedParams } from "../../lib/types";

interface Props {
  derived: DerivedParams;
}

function Row({ label, value, unit }: { label: string; value: string; unit: string }) {
  return (
    <div className="flex justify-between py-1">
      <span className="text-xs text-zinc-500">{label}</span>
      <span className="text-xs text-zinc-300">
        {value} <span className="text-zinc-500">{unit}</span>
      </span>
    </div>
  );
}

export function DerivedParamsPanel({ derived }: Props) {
  return (
    <div className="flex flex-col divide-y divide-zinc-800">
      <Row label="Wavelength" value={(derived.wavelength_m * 100).toFixed(2)} unit="cm" />
      <Row label="Chirp Rate" value={(derived.chirp_rate / 1e12).toFixed(2)} unit="THz/s" />
      <Row label="PRI" value={(derived.pri_s * 1e3).toFixed(2)} unit="ms" />
      <Row label="Max Range" value={(derived.max_unambiguous_range_m / 1e3).toFixed(1)} unit="km" />
      <Row label="Max Velocity" value={(derived.max_unambiguous_velocity_ms * 3.6).toFixed(1)} unit="km/h" />
      <Row label="Range Res." value={derived.range_resolution_m.toFixed(1)} unit="m" />
      <Row label="Velocity Res." value={(derived.velocity_resolution_ms * 3.6).toFixed(2)} unit="km/h" />
      <Row label="Packet Size" value={derived.packet_size_bytes.toFixed(0)} unit="bytes" />
      <Row label="Data Rate" value={derived.data_rate_mbps.toFixed(2)} unit="MB/s" />
    </div>
  );
}
