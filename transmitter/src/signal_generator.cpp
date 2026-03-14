#include "signal_generator.hpp"
#include <cmath>
#include <algorithm>

namespace aerotrack {

static constexpr double SPEED_OF_LIGHT = 3.0e8;  // m/s
static constexpr double PI = M_PI;
static constexpr double TWO_PI = 2.0 * M_PI;

SignalGenerator::SignalGenerator(const RadarConfig& config)
    : config_(config)
    , dt_(1.0 / config.sampling_frequency_hz)
{
}

SignalGenerator::IQVector SignalGenerator::generateChirp() const
{
    const uint32_t N = config_.num_samples_per_pulse;
    IQVector chirp(N);

    const double mu = config_.chirp_rate();   // Chirp rate: B/T
    for (uint32_t n = 0; n < N; ++n) {
        double t = n * dt_;

        // LFM chirp: s(t) = exp(j * 2π * (f₀*t + µ*t²/2))
        // Baseband'a indirilmiş hali (mixer sonrası):
        // s_bb(t) = exp(j * π * µ * t²)
        double phase = PI * mu * t * t;

        chirp[n] = IQSample(
            static_cast<float>(std::cos(phase)),
            static_cast<float>(std::sin(phase))
        );
    }

    return chirp;
}

SignalGenerator::IQVector SignalGenerator::applyRangeDelay(
    const IQVector& signal, double range_m) const
{
    const uint32_t N = signal.size();
    IQVector delayed(N, IQSample(0.0f, 0.0f));

    // Zaman gecikmesi: τ = 2R/c (gidiş-dönüş)
    double delay_s = (2.0 * range_m) / SPEED_OF_LIGHT;

    // Gecikmenin örnekleme noktalarına dönüşümü
    int delay_samples = static_cast<int>(std::round(delay_s * config_.sampling_frequency_hz));

    // Kaydırılmış sinyali yerleştir
    for (uint32_t n = 0; n < N; ++n) {
        int src_idx = static_cast<int>(n) - delay_samples;
        if (src_idx >= 0 && src_idx < static_cast<int>(N)) {
            delayed[n] = signal[src_idx];
        }
    }

    return delayed;
}

SignalGenerator::IQVector SignalGenerator::applyDopplerShift(
    const IQVector& signal, double velocity_kmh) const
{
    const uint32_t N = signal.size();
    IQVector shifted(N);

    // km/h → m/s dönüşümü
    double velocity_ms = velocity_kmh / 3.6;

    // Doppler frekansı: f_d = 2*v / λ
    double f_doppler = (2.0 * velocity_ms) / config_.wavelength();

    for (uint32_t n = 0; n < N; ++n) {
        double t = n * dt_;

        // Doppler faz kayması: exp(j * 2π * f_d * t)
        double phase = TWO_PI * f_doppler * t;

        IQSample doppler_phasor(
            static_cast<float>(std::cos(phase)),
            static_cast<float>(std::sin(phase))
        );

        shifted[n] = signal[n] * doppler_phasor;
    }

    return shifted;
}

SignalGenerator::IQVector SignalGenerator::applyAmplitudeScaling(
    const IQVector& signal, double rcs_m2, double range_m) const
{
    IQVector scaled(signal.size());

    // Radar denklemi tabanlı genlik ölçekleme
    // P_r ∝ (RCS) / (R⁴)
    // Genlik ∝ sqrt(RCS) / R²
    double amplitude = std::sqrt(rcs_m2) / (range_m * range_m);

    // Normalize (çok küçük değerleri önlemek için)
    amplitude = std::min(amplitude, 1.0);

    auto scale = static_cast<float>(amplitude);

    for (size_t n = 0; n < signal.size(); ++n) {
        scaled[n] = signal[n] * scale;
    }

    return scaled;
}

} // namespace aerotrack
