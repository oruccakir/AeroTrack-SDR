"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { api } from "../api";
import type {
  RadarConfig,
  DerivedParams,
  Target,
  SimulationStatus,
  WaveformData,
  RangeDopplerData,
  TargetSnapshot,
} from "../types";

export function useSimulation() {
  const [config, setConfig] = useState<RadarConfig | null>(null);
  const [derived, setDerived] = useState<DerivedParams | null>(null);
  const [targets, setTargets] = useState<Target[]>([]);
  const [status, setStatus] = useState<SimulationStatus>({
    state: "IDLE",
    current_cpi: 0,
    total_packets: 0,
    elapsed_s: 0,
    throughput_mbps: 0,
    max_cpi_count: 0,
  });
  const [waveform, setWaveform] = useState<WaveformData | null>(null);
  const [rangeDoppler, setRangeDoppler] = useState<RangeDopplerData | null>(null);
  const [targetHistory, setTargetHistory] = useState<TargetSnapshot[]>([]);
  const [connected, setConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const statusInterval = useRef<ReturnType<typeof setInterval>>(undefined);
  const dataInterval = useRef<ReturnType<typeof setInterval>>(undefined);

  // Initial fetch
  const fetchAll = useCallback(async () => {
    try {
      const [cfg, der, tgts, sts] = await Promise.all([
        api.getConfig(),
        api.getDerived(),
        api.getTargets(),
        api.getStatus(),
      ]);
      setConfig(cfg);
      setDerived(der);
      setTargets(tgts);
      setStatus(sts);
      setConnected(true);
      setError(null);
    } catch {
      setConnected(false);
      setError("Backend bağlantısı kurulamadı");
    }
  }, []);

  // Poll status
  useEffect(() => {
    fetchAll();

    statusInterval.current = setInterval(async () => {
      try {
        const sts = await api.getStatus();
        setStatus(sts);
        setConnected(true);
      } catch {
        setConnected(false);
      }
    }, 500);

    return () => clearInterval(statusInterval.current);
  }, [fetchAll]);

  // Poll data when running
  useEffect(() => {
    if (status.state !== "RUNNING") {
      clearInterval(dataInterval.current);
      return;
    }

    const poll = async () => {
      try {
        const [wf, rd, th] = await Promise.all([
          api.getWaveform(),
          api.getRangeDoppler(),
          api.getTargetHistory(),
        ]);
        setWaveform(wf);
        setRangeDoppler(rd);
        setTargetHistory(th);
      } catch {
        // ignore transient errors
      }
    };

    poll();
    dataInterval.current = setInterval(poll, 300);

    return () => clearInterval(dataInterval.current);
  }, [status.state]);

  // Fetch final data when stopped
  useEffect(() => {
    if (status.state === "STOPPED") {
      Promise.all([
        api.getWaveform(),
        api.getRangeDoppler(),
        api.getTargetHistory(),
      ]).then(([wf, rd, th]) => {
        setWaveform(wf);
        setRangeDoppler(rd);
        setTargetHistory(th);
      }).catch(() => {});
    }
  }, [status.state]);

  const updateConfig = useCallback(async (newConfig: Partial<RadarConfig>) => {
    try {
      await api.putConfig(newConfig);
      const [cfg, der] = await Promise.all([api.getConfig(), api.getDerived()]);
      setConfig(cfg);
      setDerived(der);
      setError(null);
    } catch (e) {
      setError(e instanceof Error ? e.message : "Config güncellenemedi");
    }
  }, []);

  const addTarget = useCallback(async (target: Partial<Target>) => {
    try {
      await api.addTarget(target);
      const tgts = await api.getTargets();
      setTargets(tgts);
      setError(null);
    } catch (e) {
      setError(e instanceof Error ? e.message : "Hedef eklenemedi");
    }
  }, []);

  const removeTarget = useCallback(async (index: number) => {
    try {
      await api.deleteTarget(index);
      const tgts = await api.getTargets();
      setTargets(tgts);
      setError(null);
    } catch (e) {
      setError(e instanceof Error ? e.message : "Hedef silinemedi");
    }
  }, []);

  const startSimulation = useCallback(async (cpiCount?: number) => {
    try {
      await api.startSimulation(cpiCount);
      setError(null);
    } catch (e) {
      setError(e instanceof Error ? e.message : "Simülasyon başlatılamadı");
    }
  }, []);

  const stopSimulation = useCallback(async () => {
    try {
      await api.stopSimulation();
      setError(null);
    } catch (e) {
      setError(e instanceof Error ? e.message : "Simülasyon durdurulamadı");
    }
  }, []);

  return {
    config,
    derived,
    targets,
    status,
    waveform,
    rangeDoppler,
    targetHistory,
    connected,
    error,
    updateConfig,
    addTarget,
    removeTarget,
    startSimulation,
    stopSimulation,
    refresh: fetchAll,
  };
}
