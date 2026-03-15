#pragma once

#include "radar_config.hpp"
#include <complex>
#include <vector>

namespace aerotrack {

/**
 * @brief LFM (Linear Frequency Modulated) chirp sinyal üreteci.
 *
 * Pulse-Doppler radar için temel chirp darbe sinyalini üretir.
 * s(t) = exp(j * 2π * (f₀*t + µ*t²/2))
 */
class SignalGenerator {
public:
  using IQSample = std::complex<float>;
  using IQVector = std::vector<IQSample>;

  explicit SignalGenerator(const RadarConfig &config);

  /**
   * @brief Temel LFM chirp darbesi üretir.
   * @return I/Q örneklerinden oluşan vektör
   */
  IQVector generateChirp() const;

  /**
   * @brief Chirp sinyaline menzil gecikmesi uygular.
   * @param signal Giriş I/Q sinyali
   * @param range_m Hedef menzili (metre)
   * @return Gecikmeli sinyal
   */
  IQVector applyRangeDelay(const IQVector &signal, double range_m) const;

  /**
   * @brief Sinyale Doppler frekans kayması uygular.
   * @param signal Giriş I/Q sinyali
   * @param velocity_kmh Hedef hızı (km/h)
   * @return Doppler kaymalı sinyal
   */
  IQVector applyDopplerShift(const IQVector &signal, double velocity_kmh) const;

  /**
   * @brief Sinyale RCS tabanlı genlik ölçekleme uygular.
   * @param signal Giriş I/Q sinyali
   * @param rcs_m2 Hedefin radar kesit alanı (m²)
   * @param range_m Hedef menzili (metre)
   * @return Ölçeklenmiş sinyal
   */
  IQVector applyAmplitudeScaling(const IQVector &signal, double rcs_m2,
                                 double range_m) const;

private:
  const RadarConfig &config_;
  double dt_; // Örnekleme periyodu
};

} // namespace aerotrack
