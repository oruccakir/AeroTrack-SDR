"use client";

import { useMemo } from "react";
import type { WaveformData } from "../../lib/types";
import { Card } from "../ui/Card";
import Plot from "./PlotlyWrapper";

interface Props {
  waveform: WaveformData | null;
}

function fftMagnitude(i: number[], q: number[]): number[] {
  const n = i.length;
  if (n === 0) return [];

  // Simple DFT for display (not performance-critical for 256 samples)
  const mag: number[] = new Array(n);
  for (let k = 0; k < n; k++) {
    let re = 0;
    let im = 0;
    for (let t = 0; t < n; t++) {
      const angle = (-2 * Math.PI * k * t) / n;
      re += i[t] * Math.cos(angle) - q[t] * Math.sin(angle);
      im += i[t] * Math.sin(angle) + q[t] * Math.cos(angle);
    }
    const m = Math.sqrt(re * re + im * im) / n;
    mag[k] = m > 1e-12 ? 20 * Math.log10(m) : -120;
  }

  // FFT shift
  const half = Math.floor(n / 2);
  const shifted = [...mag.slice(half), ...mag.slice(0, half)];
  return shifted;
}

export function SignalSpectrum({ waveform }: Props) {
  const spectrum = useMemo(() => {
    if (!waveform || waveform.sample_count === 0) return null;
    return fftMagnitude(waveform.i, waveform.q);
  }, [waveform]);

  if (!spectrum) {
    return (
      <Card title="Signal Spectrum">
        <div className="flex items-center justify-center text-xs text-zinc-600" style={{ height: 240 }}>
          No data - start simulation
        </div>
      </Card>
    );
  }

  const n = spectrum.length;
  const freqAxis = Array.from(
    { length: n },
    (_, i) => (i - n / 2) / n
  );

  return (
    <Card title="Signal Spectrum">
      <Plot
        data={[
          {
            x: freqAxis,
            y: spectrum,
            type: "scatter",
            mode: "lines",
            name: "Power (dB)",
            line: { color: "#f59e0b", width: 1 },
            fill: "tozeroy",
            fillcolor: "rgba(245, 158, 11, 0.1)",
          },
        ]}
        layout={{
          margin: { t: 10, r: 10, b: 40, l: 50 },
          paper_bgcolor: "transparent",
          plot_bgcolor: "transparent",
          font: { color: "#a1a1aa", size: 10 },
          xaxis: {
            title: { text: "Normalized Frequency" },
            gridcolor: "#27272a",
            zerolinecolor: "#3f3f46",
          },
          yaxis: {
            title: { text: "Power (dB)" },
            gridcolor: "#27272a",
            zerolinecolor: "#3f3f46",
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
