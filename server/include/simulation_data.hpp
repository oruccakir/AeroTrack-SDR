#pragma once

#include "radar_config.hpp"
#include "target_simulator.hpp"
#include <complex>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace aerotrack {

enum class SimState { IDLE, RUNNING, STOPPED };

inline std::string simStateToString(SimState s) {
  switch (s) {
  case SimState::IDLE:
    return "IDLE";
  case SimState::RUNNING:
    return "RUNNING";
  case SimState::STOPPED:
    return "STOPPED";
  }
  return "UNKNOWN";
}

struct TargetSnapshot {
  uint32_t cpi_index;
  double elapsed_s;
  std::vector<Target> targets;
};

struct SimulationData {
  mutable std::mutex mtx;

  // State
  SimState state = SimState::IDLE;

  // Config
  RadarConfig config;
  std::vector<Target> targets;

  // Live stats
  uint32_t current_cpi = 0;
  uint64_t total_packets = 0;
  double elapsed_s = 0.0;
  double throughput_mbps = 0.0;

  // Data buffers
  std::deque<TargetSnapshot> target_history; // son 200 CPI
  std::vector<std::complex<float>> latest_iq_waveform;
  std::vector<float> range_doppler_map; // flattened 2D: [range_bin * doppler_bins + doppler_bin]
  uint32_t rd_range_bins = 0;
  uint32_t rd_doppler_bins = 0;

  // Max CPI limit (0 = infinite)
  uint32_t max_cpi_count = 0;

  // UDP enable flag
  bool udp_enabled = false;

  void addTargetSnapshot(uint32_t cpi, double t,
                         const std::vector<Target> &tgts) {
    // caller must hold mtx
    target_history.push_back({cpi, t, tgts});
    if (target_history.size() > 200) {
      target_history.pop_front();
    }
  }
};

} // namespace aerotrack
