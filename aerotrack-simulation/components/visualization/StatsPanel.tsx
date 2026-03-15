import type { SimulationStatus } from "../../lib/types";
import { Card } from "../ui/Card";

interface Props {
  status: SimulationStatus;
}

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div className="flex flex-col items-center">
      <span className="text-lg font-mono font-bold text-zinc-100">{value}</span>
      <span className="text-xs text-zinc-500">{label}</span>
    </div>
  );
}

export function StatsPanel({ status }: Props) {
  return (
    <Card title="Statistics">
      <div className="grid grid-cols-4 gap-4">
        <Stat label="CPI" value={status.current_cpi.toLocaleString()} />
        <Stat label="Packets" value={status.total_packets.toLocaleString()} />
        <Stat label="Time" value={`${status.elapsed_s.toFixed(1)}s`} />
        <Stat label="Throughput" value={`${status.throughput_mbps.toFixed(2)} MB/s`} />
      </div>
    </Card>
  );
}
