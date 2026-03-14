#pragma once

#include "radar_config.hpp"
#include "signal_generator.hpp"
#include <complex>
#include <vector>
#include <cstdint>
#include <string>

namespace aerotrack {

/**
 * @brief Simüle edilen radar hedefi.
 */
struct Target {
    double range_m;        // Menzil (metre)
    double velocity_kmh;   // Hız (km/h, pozitif = yaklaşan)
    double rcs_m2;         // Radar Kesit Alanı (m²)
    double azimuth_deg;    // Azimut açısı (derece)

    std::string name;      // Hedef etiketi

    Target(double range, double velocity, double rcs,
           double azimuth = 0.0, const std::string& label = "Target")
        : range_m(range)
        , velocity_kmh(velocity)
        , rcs_m2(rcs)
        , azimuth_deg(azimuth)
        , name(label) {}
};

/**
 * @brief Birden fazla hedefin radar yansımalarını simüle eder.
 *
 * Her darbe (pulse) için tüm hedeflerden gelen eko sinyallerini
 * üretir ve birleştirir.
 */
class TargetSimulator {
public:
    using IQSample = std::complex<float>;
    using IQVector = std::vector<IQSample>;

    explicit TargetSimulator(const RadarConfig& config);

    /**
     * @brief Hedef listesine yeni hedef ekler.
     */
    void addTarget(const Target& target);

    /**
     * @brief Tüm hedefleri temizler.
     */
    void clearTargets();

    /**
     * @brief Bir darbe (pulse) için tüm hedeflerden gelen toplam eko sinyalini üretir.
     * @param pulse_index CPI içindeki darbe indexi
     * @return Birleşik I/Q eko sinyali
     */
    IQVector simulatePulse(uint32_t pulse_index) const;

    /**
     * @brief Hedef pozisyonlarını günceller (hız × Δt).
     * @param delta_time_s Geçen süre (saniye)
     */
    void updateTargetPositions(double delta_time_s);

    /**
     * @brief Hedef listesini döndürür.
     */
    const std::vector<Target>& getTargets() const { return targets_; }

    /**
     * @brief Hedef sayısını döndürür.
     */
    size_t getTargetCount() const { return targets_.size(); }

private:
    const RadarConfig& config_;
    SignalGenerator signal_gen_;
    std::vector<Target> targets_;
};

} // namespace aerotrack
