"use client";

import type { TargetSnapshot } from "../../lib/types";
import { Card } from "../ui/Card";
import Plot from "./PlotlyWrapper";

interface Props {
  history: TargetSnapshot[];
}

export function RadarPPI({ history }: Props) {
  const latest = history.length > 0 ? history[history.length - 1] : null;

  if (!latest || latest.targets.length === 0) {
    return (
      <Card title="Radar PPI">
        <div className="flex items-center justify-center text-xs text-zinc-600" style={{ height: 240 }}>
          No data - start simulation
        </div>
      </Card>
    );
  }

  const r = latest.targets.map((t) => t.range_m);
  const theta = latest.targets.map((t) => t.azimuth_deg);
  const labels = latest.targets.map(
    (t) => `${t.name}<br>${t.range_m.toFixed(0)}m | ${t.velocity_kmh.toFixed(0)}km/h`
  );

  // Also show trails from history
  const trailR: number[] = [];
  const trailTheta: number[] = [];
  const numTrail = Math.min(history.length, 50);
  for (let i = history.length - numTrail; i < history.length; i++) {
    for (const t of history[i].targets) {
      trailR.push(t.range_m);
      trailTheta.push(t.azimuth_deg);
    }
  }

  return (
    <Card title="Radar PPI">
      <Plot
        data={[
          {
            type: "scatterpolar",
            mode: "markers",
            r: trailR,
            theta: trailTheta,
            marker: { color: "#10b98133", size: 3 },
            showlegend: false,
          },
          {
            type: "scatterpolar",
            mode: "text+markers",
            r,
            theta,
            text: labels,
            textposition: "top center",
            textfont: { size: 8, color: "#a1a1aa" },
            marker: { color: "#10b981", size: 10, symbol: "diamond" },
            name: "Targets",
          },
        ]}
        layout={{
          margin: { t: 20, r: 30, b: 20, l: 30 },
          paper_bgcolor: "transparent",
          plot_bgcolor: "transparent",
          font: { color: "#a1a1aa", size: 10 },
          polar: {
            bgcolor: "transparent",
            radialaxis: {
              gridcolor: "#27272a",
              linecolor: "#3f3f46",
              tickfont: { color: "#71717a" },
            },
            angularaxis: {
              gridcolor: "#27272a",
              linecolor: "#3f3f46",
              tickfont: { color: "#71717a" },
              direction: "clockwise",
            },
          },
          showlegend: false,
          autosize: true,
        }}
        config={{ responsive: true, displayModeBar: false }}
        style={{ width: "100%", height: 240 }}
        useResizeHandler
      />
    </Card>
  );
}
