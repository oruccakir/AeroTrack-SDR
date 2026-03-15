"use client";

import { useState } from "react";
import type { RadarConfig } from "../../lib/types";
import { Input } from "../ui/Input";
import { Button } from "../ui/Button";

interface Props {
  config: RadarConfig;
  onUpdate: (config: Partial<RadarConfig>) => Promise<void>;
  disabled: boolean;
}

export function RadarConfigForm({ config, onUpdate, disabled }: Props) {
  const [form, setForm] = useState(config);

  const set = (key: keyof RadarConfig, value: string) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      setForm((prev) => ({ ...prev, [key]: num }));
    }
  };

  const handleApply = () => {
    onUpdate(form);
  };

  return (
    <div className="flex flex-col gap-3">
      <h4 className="text-xs font-semibold uppercase tracking-wider text-zinc-500">
        RF Parameters
      </h4>
      <Input
        label="Carrier Frequency"
        unit="GHz"
        type="number"
        step="0.1"
        value={form.carrier_frequency_hz / 1e9}
        onChange={(e) =>
          setForm((p) => ({
            ...p,
            carrier_frequency_hz: parseFloat(e.target.value) * 1e9,
          }))
        }
        disabled={disabled}
      />
      <Input
        label="Bandwidth"
        unit="MHz"
        type="number"
        step="0.1"
        value={form.bandwidth_hz / 1e6}
        onChange={(e) =>
          setForm((p) => ({
            ...p,
            bandwidth_hz: parseFloat(e.target.value) * 1e6,
          }))
        }
        disabled={disabled}
      />
      <Input
        label="Pulse Duration"
        unit="us"
        type="number"
        step="1"
        value={form.pulse_duration_s * 1e6}
        onChange={(e) =>
          setForm((p) => ({
            ...p,
            pulse_duration_s: parseFloat(e.target.value) * 1e-6,
          }))
        }
        disabled={disabled}
      />

      <h4 className="mt-2 text-xs font-semibold uppercase tracking-wider text-zinc-500">
        Timing
      </h4>
      <Input
        label="PRF"
        unit="Hz"
        type="number"
        step="100"
        value={form.prf_hz}
        onChange={(e) => set("prf_hz", e.target.value)}
        disabled={disabled}
      />
      <Input
        label="Pulses / CPI"
        type="number"
        step="1"
        value={form.num_pulses_per_cpi}
        onChange={(e) => set("num_pulses_per_cpi", e.target.value)}
        disabled={disabled}
      />
      <Input
        label="Samples / Pulse"
        type="number"
        step="1"
        value={form.num_samples_per_pulse}
        onChange={(e) => set("num_samples_per_pulse", e.target.value)}
        disabled={disabled}
      />

      <h4 className="mt-2 text-xs font-semibold uppercase tracking-wider text-zinc-500">
        Signal
      </h4>
      <Input
        label="SNR"
        unit="dB"
        type="number"
        step="1"
        value={form.snr_db}
        onChange={(e) => set("snr_db", e.target.value)}
        disabled={disabled}
      />

      <Button
        onClick={handleApply}
        disabled={disabled}
        className="mt-3 w-full"
      >
        Apply Config
      </Button>
    </div>
  );
}
