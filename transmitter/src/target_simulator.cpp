#include "target_simulator.hpp"
#include <cmath>
#include <iostream>

namespace aerotrack {

TargetSimulator::TargetSimulator(const RadarConfig &config)
    : config_(config), signal_gen_(config) {}

void TargetSimulator::addTarget(const Target &target) {
  targets_.push_back(target);
  std::cout << "[TargetSim] Hedef eklendi: \"" << target.name
            << "\" | Menzil: " << target.range_m << " m"
            << " | Hız: " << target.velocity_kmh << " km/h"
            << " | RCS: " << target.rcs_m2 << " m²" << std::endl;
}

void TargetSimulator::clearTargets() { targets_.clear(); }

TargetSimulator::IQVector
TargetSimulator::simulatePulse(uint32_t pulse_index) const {
  const uint32_t N = config_.num_samples_per_pulse;
  IQVector combined(N, IQSample(0.0f, 0.0f));

  // Temel chirp darbesi
  IQVector base_chirp = signal_gen_.generateChirp();

  for (const auto &target : targets_) {
    // 1. Menzil gecikmesi uygula
    IQVector echo = signal_gen_.applyRangeDelay(base_chirp, target.range_m);

    // 2. Doppler frekans kayması uygula
    echo = signal_gen_.applyDopplerShift(echo, target.velocity_kmh);

    // 3. RCS ve menzile göre genlik ölçekle
    echo =
        signal_gen_.applyAmplitudeScaling(echo, target.rcs_m2, target.range_m);

    // 4. Darbe-arası Doppler faz ilerlemesi
    // Her ardışık darbede ek faz birikir: φ = 2π * f_d * pulse_index * PRI
    double velocity_ms = target.velocity_kmh / 3.6;
    double f_doppler = (2.0 * velocity_ms) / config_.wavelength();
    double inter_pulse_phase =
        2.0 * M_PI * f_doppler * pulse_index * config_.pri_s();

    IQSample phase_factor(static_cast<float>(std::cos(inter_pulse_phase)),
                          static_cast<float>(std::sin(inter_pulse_phase)));

    // 5. Toplam sinyale ekle
    for (uint32_t n = 0; n < N; ++n) {
      combined[n] += echo[n] * phase_factor;
    }
  }

  return combined;
}

void TargetSimulator::updateTargetPositions(double delta_time_s) {
  for (auto &target : targets_) {
    // km/h → m/s dönüşümü
    double velocity_ms = target.velocity_kmh / 3.6;

    // Menzil güncelle (pozitif hız = yaklaşan, menzil azalır)
    target.range_m -= velocity_ms * delta_time_s;

    // Negatif menzil kontrolü
    if (target.range_m < 1.0) {
      target.range_m = 1.0;
      target.velocity_kmh = 0.0;
      std::cout << "[TargetSim] UYARI: \"" << target.name
                << "\" minimum menzile ulaştı!" << std::endl;
    }
  }
}

} // namespace aerotrack
