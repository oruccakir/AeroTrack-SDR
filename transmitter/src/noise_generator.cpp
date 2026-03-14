#include "noise_generator.hpp"
#include <cmath>

namespace aerotrack {

NoiseGenerator::NoiseGenerator(uint32_t seed) : rng_(seed), dist_(0.0f, 1.0f) {}

double NoiseGenerator::computeSignalPower(const IQVector &signal) const {
  if (signal.empty())
    return 0.0;

  double power = 0.0;
  for (const auto &sample : signal) {
    power += std::norm(sample); // |s|² = I² + Q²
  }
  return power / static_cast<double>(signal.size());
}

NoiseGenerator::IQVector NoiseGenerator::addNoise(const IQVector &signal,
                                                  double snr_db) const {
  if (signal.empty())
    return signal;

  // Sinyal gücünü hesapla
  double signal_power = computeSignalPower(signal);

  // SNR'den gürültü gücünü hesapla
  // SNR_dB = 10 * log10(P_signal / P_noise)
  // P_noise = P_signal / 10^(SNR_dB/10)
  double noise_power = signal_power / std::pow(10.0, snr_db / 10.0);

  // Gürültü standart sapması (her bir I/Q bileşeni için)
  // Toplam gürültü gücü = σ_I² + σ_Q² = 2*σ²
  // σ = sqrt(P_noise / 2)
  float noise_std = static_cast<float>(std::sqrt(noise_power / 2.0));

  IQVector noisy_signal(signal.size());

  for (size_t n = 0; n < signal.size(); ++n) {
    float noise_i = dist_(rng_) * noise_std;
    float noise_q = dist_(rng_) * noise_std;

    noisy_signal[n] = signal[n] + IQSample(noise_i, noise_q);
  }

  return noisy_signal;
}

NoiseGenerator::IQVector
NoiseGenerator::generateNoise(size_t num_samples, double noise_power) const {
  IQVector noise(num_samples);

  float noise_std = static_cast<float>(std::sqrt(noise_power / 2.0));

  for (size_t n = 0; n < num_samples; ++n) {
    noise[n] = IQSample(dist_(rng_) * noise_std, dist_(rng_) * noise_std);
  }

  return noise;
}

} // namespace aerotrack
