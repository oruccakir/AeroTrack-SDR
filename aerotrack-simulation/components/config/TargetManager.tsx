"use client";

import { useState } from "react";
import type { Target } from "../../lib/types";
import { Input } from "../ui/Input";
import { Button } from "../ui/Button";

interface Props {
  targets: Target[];
  onAdd: (target: Partial<Target>) => Promise<void>;
  onRemove: (index: number) => Promise<void>;
  disabled: boolean;
}

export function TargetManager({ targets, onAdd, onRemove, disabled }: Props) {
  const [range, setRange] = useState("5000");
  const [velocity, setVelocity] = useState("300");
  const [rcs, setRcs] = useState("10");
  const [azimuth, setAzimuth] = useState("0");

  const handleAdd = () => {
    onAdd({
      range_m: parseFloat(range),
      velocity_kmh: parseFloat(velocity),
      rcs_m2: parseFloat(rcs),
      azimuth_deg: parseFloat(azimuth),
    });
  };

  return (
    <div className="flex flex-col gap-3">
      <h4 className="text-xs font-semibold uppercase tracking-wider text-zinc-500">
        Add Target
      </h4>
      <div className="grid grid-cols-2 gap-2">
        <Input label="Range" unit="m" type="number" value={range} onChange={(e) => setRange(e.target.value)} disabled={disabled} />
        <Input label="Velocity" unit="km/h" type="number" value={velocity} onChange={(e) => setVelocity(e.target.value)} disabled={disabled} />
        <Input label="RCS" unit="m2" type="number" value={rcs} onChange={(e) => setRcs(e.target.value)} disabled={disabled} />
        <Input label="Azimuth" unit="deg" type="number" value={azimuth} onChange={(e) => setAzimuth(e.target.value)} disabled={disabled} />
      </div>
      <Button onClick={handleAdd} disabled={disabled} size="sm" className="w-full">
        Add Target
      </Button>

      <h4 className="mt-3 text-xs font-semibold uppercase tracking-wider text-zinc-500">
        Target List ({targets.length})
      </h4>
      {targets.length === 0 ? (
        <p className="text-xs text-zinc-600">No targets added</p>
      ) : (
        <div className="flex flex-col gap-1">
          {targets.map((t, i) => (
            <div
              key={i}
              className="flex items-center justify-between rounded border border-zinc-800 bg-zinc-800/50 px-3 py-2"
            >
              <div className="text-xs">
                <span className="text-zinc-300">{t.name}</span>
                <span className="ml-2 text-zinc-500">
                  {t.range_m.toFixed(0)}m | {t.velocity_kmh.toFixed(0)}km/h | {t.rcs_m2}m2
                </span>
              </div>
              <button
                onClick={() => onRemove(i)}
                disabled={disabled}
                className="text-xs text-red-400 hover:text-red-300 disabled:opacity-50"
              >
                X
              </button>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
