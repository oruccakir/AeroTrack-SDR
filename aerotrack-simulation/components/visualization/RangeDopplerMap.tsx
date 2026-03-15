"use client";

import type { RangeDopplerData, DerivedParams } from "../../lib/types";
import { Card } from "../ui/Card";
import Plot from "./PlotlyWrapper";

interface Props {
  data: RangeDopplerData | null;
  derived: DerivedParams | null;
}

export function RangeDopplerMap({ data, derived }: Props) {
  if (!data || data.range_bins === 0 || data.doppler_bins === 0) {
    return (
      <Card title="Range-Doppler Map">
        <div className="flex items-center justify-center text-xs text-zinc-600" style={{ height: 280 }}>
          No data - start simulation
        </div>
      </Card>
    );
  }

  // Convert flat array to 2D
  const z: number[][] = [];
  for (let r = 0; r < data.range_bins; r++) {
    const row: number[] = [];
    for (let d = 0; d < data.doppler_bins; d++) {
      row.push(data.data[r * data.doppler_bins + d]);
    }
    z.push(row);
  }

  // Axis labels
  const maxRange = derived ? derived.max_unambiguous_range_m : data.range_bins;
  const maxVel = derived ? derived.max_unambiguous_velocity_ms * 3.6 : data.doppler_bins;

  const rangeAxis = Array.from(
    { length: data.range_bins },
    (_, i) => (i / data.range_bins) * maxRange
  );
  const dopplerAxis = Array.from(
    { length: data.doppler_bins },
    (_, i) => -maxVel + (2 * maxVel * i) / data.doppler_bins
  );

  return (
    <Card title="Range-Doppler Map">
      <Plot
        data={[
          {
            z,
            x: dopplerAxis,
            y: rangeAxis,
            type: "heatmap",
            colorscale: "Jet",
            colorbar: {
              title: { text: "dB", font: { color: "#a1a1aa" } },
              tickfont: { color: "#a1a1aa" },
            },
            zmin: -80,
            zmax: 0,
          },
        ]}
        layout={{
          margin: { t: 10, r: 10, b: 50, l: 60 },
          paper_bgcolor: "transparent",
          plot_bgcolor: "transparent",
          font: { color: "#a1a1aa", size: 10 },
          xaxis: {
            title: { text: "Velocity (km/h)" },
            gridcolor: "#27272a",
          },
          yaxis: {
            title: { text: "Range (m)" },
            gridcolor: "#27272a",
          },
          autosize: true,
        }}
        config={{ responsive: true, displayModeBar: false }}
        style={{ width: "100%", height: 280 }}
        useResizeHandler
      />
    </Card>
  );
}
