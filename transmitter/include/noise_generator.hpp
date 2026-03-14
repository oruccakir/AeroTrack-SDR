#pragma once

#include <complex>
#include <vector>
#include <random>

namespace aerotrack {

/**
 * @brief AWGN (Additive White Gaussian Noise) üreteci.
 *
 * Belirtilen SNR seviyesine göre I/Q sinyaline
 * Gauss gürültüsü ekler.
 */
class NoiseGenerator {
public:
    using IQSample = std::complex<float>;
    using IQVector = std::vector<IQSample>;

    /**
     * @param seed Rastgele sayı üreteci tohumu (tekrarlanabilirlik için)
     */
    explicit NoiseGenerator(uint32_t seed = 42);

    /**
     * @brief Sinyale belirtilen SNR'ye göre AWGN ekler.
     * @param signal Giriş I/Q sinyali
     * @param snr_db Sinyal-Gürültü Oranı (dB)
     * @return Gürültülü sinyal
     */
    IQVector addNoise(const IQVector& signal, double snr_db) const;

    /**
     * @brief Yalnızca gürültü vektörü üretir.
     * @param num_samples Örnek sayısı
     * @param noise_power Gürültü gücü (lineer)
     * @return Gürültü vektörü
     */
    IQVector generateNoise(size_t num_samples, double noise_power) const;

private:
    mutable std::mt19937 rng_;
    mutable std::normal_distribution<float> dist_;

    /**
     * @brief Sinyalin ortalama gücünü hesaplar.
     */
    double computeSignalPower(const IQVector& signal) const;
};

} // namespace aerotrack
