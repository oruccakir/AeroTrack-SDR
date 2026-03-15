import { Badge } from "../ui/Badge";

interface HeaderProps {
  connected: boolean;
  state: "IDLE" | "RUNNING" | "STOPPED";
}

export function Header({ connected, state }: HeaderProps) {
  const stateVariant = state === "RUNNING" ? "running" : state === "STOPPED" ? "stopped" : "idle";

  return (
    <header className="flex items-center justify-between border-b border-zinc-800 bg-zinc-900/90 px-6 py-3 backdrop-blur">
      <div className="flex items-center gap-3">
        <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-emerald-600 text-sm font-bold text-white">
          A
        </div>
        <h1 className="text-lg font-semibold text-zinc-100">AeroTrack-SDR</h1>
        <span className="text-xs text-zinc-500">Radar Simulation Dashboard</span>
      </div>
      <div className="flex items-center gap-3">
        <Badge variant={stateVariant}>{state}</Badge>
        <Badge variant={connected ? "connected" : "disconnected"}>
          {connected ? "Connected" : "Disconnected"}
        </Badge>
      </div>
    </header>
  );
}
