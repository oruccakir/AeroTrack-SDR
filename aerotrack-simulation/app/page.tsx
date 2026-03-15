"use client";

import { useSimulation } from "../lib/hooks/useSimulation";
import { Header } from "../components/layout/Header";
import { Sidebar } from "../components/layout/Sidebar";
import { StatsPanel } from "../components/visualization/StatsPanel";
import { RangeDopplerMap } from "../components/visualization/RangeDopplerMap";
import { IQWaveform } from "../components/visualization/IQWaveform";
import { TargetTracker } from "../components/visualization/TargetTracker";
import { RadarPPI } from "../components/visualization/RadarPPI";
import { SignalSpectrum } from "../components/visualization/SignalSpectrum";

export default function Dashboard() {
  const sim = useSimulation();

  return (
    <div className="flex h-screen flex-col overflow-hidden">
      <Header connected={sim.connected} state={sim.status.state} />

      <div className="flex flex-1 overflow-hidden">
        <Sidebar
          config={sim.config}
          derived={sim.derived}
          targets={sim.targets}
          state={sim.status.state}
          onUpdateConfig={sim.updateConfig}
          onAddTarget={sim.addTarget}
          onRemoveTarget={sim.removeTarget}
          onStart={sim.startSimulation}
          onStop={sim.stopSimulation}
          error={sim.error}
        />

        <main className="flex-1 overflow-y-auto p-4">
          <div className="flex flex-col gap-4">
            {/* Row 1: Range-Doppler + I/Q Waveform */}
            <div className="grid grid-cols-1 gap-4 lg:grid-cols-2">
              <RangeDopplerMap data={sim.rangeDoppler} derived={sim.derived} />
              <IQWaveform waveform={sim.waveform} />
            </div>

            {/* Row 2: Target Tracker + Radar PPI */}
            <div className="grid grid-cols-1 gap-4 lg:grid-cols-2">
              <TargetTracker history={sim.targetHistory} />
              <RadarPPI history={sim.targetHistory} />
            </div>

            {/* Row 3: Stats + Signal Spectrum */}
            <div className="grid grid-cols-1 gap-4 lg:grid-cols-2">
              <StatsPanel status={sim.status} />
              <SignalSpectrum waveform={sim.waveform} />
            </div>
          </div>
        </main>
      </div>
    </div>
  );
}
