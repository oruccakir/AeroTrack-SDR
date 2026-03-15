"use client";

import type { WaveformData } from "../../lib/types";
import { Card } from "../ui/Card";
import Plot from "./PlotlyWrapper";

interface Props {
  waveform: WaveformData | null;
}

export function IQWaveform({ waveform }: Props) {
  if (!waveform || waveform.sample_count === 0) {
    return (
      <Card title="I/Q Waveform">
        <div className="flex items-center justify-center text-xs text-zinc-600" style={{ height: 280 }}>
          No data - start simulation
        </div>
      </Card>
    );
  }

  const x = Array.from({ length: waveform.sample_count }, (_, i) => i);

  return (
    <Card title="I/Q Waveform">
      <Plot
        data={[
          {
            x,
            y: waveform.i,
            type: "scatter",
            mode: "lines",
            name: "I (In-phase)",
            line: { color: "#10b981", width: 1 },
          },
          {
            x,
            y: waveform.q,
            type: "scatter",
            mode: "lines",
            name: "Q (Quadrature)",
            line: { color: "#6366f1", width: 1 },
          },
        ]}
        layout={{
          margin: { t: 10, r: 10, b: 40, l: 50 },
          paper_bgcolor: "transparent",
          plot_bgcolor: "transparent",
          font: { color: "#a1a1aa", size: 10 },
          xaxis: {
            title: { text: "Sample Index" },
            gridcolor: "#27272a",
            zerolinecolor: "#3f3f46",
          },
          yaxis: {
            title: { text: "Amplitude" },
            gridcolor: "#27272a",
            zerolinecolor: "#3f3f46",
          },
          legend: { x: 0, y: 1.15, orientation: "h" },
          showlegend: true,
          autosize: true,
        }}
        config={{ responsive: true, displayModeBar: false }}
        style={{ width: "100%", height: 280 }}
        useResizeHandler
      />
    </Card>
  );
}
