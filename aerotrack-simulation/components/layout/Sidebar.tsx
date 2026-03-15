"use client";

import { useState } from "react";
import type { RadarConfig, DerivedParams, Target } from "../../lib/types";
import { RadarConfigForm } from "../config/RadarConfigForm";
import { TargetManager } from "../config/TargetManager";
import { DerivedParamsPanel } from "../config/DerivedParams";
import { Button } from "../ui/Button";

interface SidebarProps {
  config: RadarConfig | null;
  derived: DerivedParams | null;
  targets: Target[];
  state: "IDLE" | "RUNNING" | "STOPPED";
  onUpdateConfig: (config: Partial<RadarConfig>) => Promise<void>;
  onAddTarget: (target: Partial<Target>) => Promise<void>;
  onRemoveTarget: (index: number) => Promise<void>;
  onStart: (cpiCount?: number) => Promise<void>;
  onStop: () => Promise<void>;
  error: string | null;
}

export function Sidebar({
  config,
  derived,
  targets,
  state,
  onUpdateConfig,
  onAddTarget,
  onRemoveTarget,
  onStart,
  onStop,
  error,
}: SidebarProps) {
  const [tab, setTab] = useState<"config" | "targets" | "derived">("config");
  const isRunning = state === "RUNNING";

  return (
    <aside className="flex h-full w-80 flex-col border-r border-zinc-800 bg-zinc-900/60">
      {/* Simulation Controls */}
      <div className="border-b border-zinc-800 p-4">
        <div className="flex gap-2">
          {isRunning ? (
            <Button variant="danger" className="flex-1" onClick={() => onStop()}>
              Stop
            </Button>
          ) : (
            <Button variant="primary" className="flex-1" onClick={() => onStart()}>
              Start
            </Button>
          )}
          <Button
            variant="ghost"
            onClick={() => onStart(100)}
            disabled={isRunning}
            title="100 CPI çalıştır"
          >
            100 CPI
          </Button>
        </div>
        {error && (
          <p className="mt-2 text-xs text-red-400">{error}</p>
        )}
      </div>

      {/* Tabs */}
      <div className="flex border-b border-zinc-800">
        {(["config", "targets", "derived"] as const).map((t) => (
          <button
            key={t}
            onClick={() => setTab(t)}
            className={`flex-1 py-2 text-xs font-medium transition-colors ${
              tab === t
                ? "border-b-2 border-emerald-500 text-emerald-400"
                : "text-zinc-500 hover:text-zinc-300"
            }`}
          >
            {t === "config" ? "Config" : t === "targets" ? "Targets" : "Derived"}
          </button>
        ))}
      </div>

      {/* Tab Content */}
      <div className="flex-1 overflow-y-auto p-4">
        {tab === "config" && config && (
          <RadarConfigForm
            config={config}
            onUpdate={onUpdateConfig}
            disabled={isRunning}
          />
        )}
        {tab === "targets" && (
          <TargetManager
            targets={targets}
            onAdd={onAddTarget}
            onRemove={onRemoveTarget}
            disabled={isRunning}
          />
        )}
        {tab === "derived" && derived && <DerivedParamsPanel derived={derived} />}
      </div>
    </aside>
  );
}
