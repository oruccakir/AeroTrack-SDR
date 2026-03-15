interface BadgeProps {
  variant: "idle" | "running" | "stopped" | "connected" | "disconnected";
  children: React.ReactNode;
}

export function Badge({ variant, children }: BadgeProps) {
  const styles = {
    idle: "bg-zinc-700 text-zinc-300",
    running: "bg-emerald-900 text-emerald-300 animate-pulse",
    stopped: "bg-amber-900 text-amber-300",
    connected: "bg-emerald-900 text-emerald-300",
    disconnected: "bg-red-900 text-red-300",
  };

  return (
    <span
      className={`inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-medium ${styles[variant]}`}
    >
      {children}
    </span>
  );
}
