#pragma once

#include <cstdint>
#include <string>
#include <cmath>

namespace aerotrack {

/**
 * @brief Radar system configuration parameters.
 *
 * Default values are set for a typical X-band pulse-Doppler radar.
 * Adjust these parameters according to your specific hardware setup.
 */
struct RadarConfig {
    // --- RF Parametreleri ---
    double carrier_frequency_hz  = 10.0e9;   // Taşıyıcı frekans (10 GHz, X-band)
    double bandwidth_hz          = 5.0e6;    // Chirp bant genişliği (5 MHz)
    double pulse_duration_s      = 10.0e-6;  // Darbe süresi (10 µs)

    // --- Zamanlama Parametreleri ---
    double prf_hz                = 1000.0;   // Pulse Repetition Frequency (1 kHz)
    uint32_t num_pulses_per_cpi  = 64;       // Bir CPI'daki darbe sayısı
    uint32_t num_samples_per_pulse = 256;    // Darbe başına I/Q örnekleme sayısı

    // --- Örnekleme ---
    double sampling_frequency_hz = 25.6e6;   // ADC örnekleme frekansı

    // --- Gürültü ---
    double snr_db                = 20.0;     // Sinyal-Gürültü Oranı (dB)

    // --- Ağ Ayarları ---
    std::string target_ip        = "192.168.1.10";
    uint16_t    target_port      = 5000;

    // --- Türetilmiş Parametreler ---
    double wavelength() const {
        return 3.0e8 / carrier_frequency_hz;  // λ = c / f
    }

    double chirp_rate() const {
        return bandwidth_hz / pulse_duration_s;  // µ = B / T
    }

    double pri_s() const {
        return 1.0 / prf_hz;  // Pulse Repetition Interval
    }

    double max_unambiguous_range() const {
        return (3.0e8 * pri_s()) / 2.0;  // R_max = c * PRI / 2
    }

    double max_unambiguous_velocity() const {
        return (wavelength() * prf_hz) / 4.0;  // V_max = λ * PRF / 4
    }

    double range_resolution() const {
        return 3.0e8 / (2.0 * bandwidth_hz);  // ΔR = c / (2B)
    }

    double velocity_resolution() const {
        return wavelength() / (2.0 * num_pulses_per_cpi * pri_s());
    }
};

} // namespace aerotrack
