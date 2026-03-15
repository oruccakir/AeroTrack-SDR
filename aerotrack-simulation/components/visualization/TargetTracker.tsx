"use client";

import type { TargetSnapshot } from "../../lib/types";
import { Card } from "../ui/Card";
import Plot from "./PlotlyWrapper";

interface Props {
  history: TargetSnapshot[];
}

export function TargetTracker({ history }: Props) {
  if (history.length === 0) {
    return (
      <Card title="Target Tracker">
        <div className="flex items-center justify-center text-xs text-zinc-600" style={{ height: 240 }}>
          No data - start simulation
        </div>
      </Card>
    );
  }

  // Collect per-target traces
  const targetNames = new Set<string>();
  for (const snap of history) {
    for (const t of snap.targets) {
      targetNames.add(t.name);
    }
  }

  const colors = ["#10b981", "#6366f1", "#f59e0b", "#ef4444", "#8b5cf6"];
  const rangeTraces: object[] = [];
  const velTraces: object[] = [];

  let colorIdx = 0;
  for (const name of targetNames) {
    const times: number[] = [];
    const ranges: number[] = [];
    const velocities: number[] = [];

    for (const snap of history) {
      const t = snap.targets.find((tgt) => tgt.name === name);
      if (t) {
        times.push(snap.elapsed_s);
        ranges.push(t.range_m);
        velocities.push(t.velocity_kmh);
      }
    }

    const color = colors[colorIdx % colors.length];
    rangeTraces.push({
      x: times,
      y: ranges,
      type: "scatter",
      mode: "lines",
      name: `${name} range`,
      line: { color, width: 1.5 },
    });
    velTraces.push({
      x: times,
      y: velocities,
      type: "scatter",
      mode: "lines",
      name: `${name} vel`,
      line: { color, width: 1.5, dash: "dash" },
      yaxis: "y2",
    });
    colorIdx++;
  }

  return (
    <Card title="Target Tracker">
      <Plot
        data={[...rangeTraces, ...velTraces]}
        layout={{
          margin: { t: 10, r: 60, b: 40, l: 60 },
          paper_bgcolor: "transparent",
          plot_bgcolor: "transparent",
          font: { color: "#a1a1aa", size: 10 },
          xaxis: { title: { text: "Time (s)" }, gridcolor: "#27272a" },
          yaxis: { title: { text: "Range (m)" }, gridcolor: "#27272a", side: "left" },
          yaxis2: {
            title: { text: "Velocity (km/h)" },
            overlaying: "y",
            side: "right",
            gridcolor: "#27272a",
          },
          legend: { x: 0, y: 1.15, orientation: "h" },
          showlegend: true,
          autosize: true,
        }}
        config={{ responsive: true, displayModeBar: false }}
        style={{ width: "100%", height: 240 }}
        useResizeHandler
      />
    </Card>
  );
}
