#pragma once

#include <complex>
#include <cstdint>
#include <vector>

namespace aerotrack {

/**
 * @brief Radix-2 DIT FFT ve Range-Doppler map hesaplayıcı.
 */
class RangeDopplerProcessor {
public:
  using Complex = std::complex<float>;

  /**
   * @brief CPI verisinden Range-Doppler map hesaplar.
   * @param cpi_data 2D veri [pulse_index][sample_index], flattened
   * @param num_pulses Darbe sayısı (2'nin kuvveti)
   * @param num_samples Darbe başına örnek sayısı (2'nin kuvveti)
   * @param ref_chirp Referans chirp (matched filter için)
   * @return dB cinsinden Range-Doppler map [range_bin * num_pulses + doppler_bin]
   */
  static std::vector<float> compute(const std::vector<Complex> &cpi_data,
                                    uint32_t num_pulses, uint32_t num_samples,
                                    const std::vector<Complex> &ref_chirp);

  /**
   * @brief In-place radix-2 DIT FFT.
   */
  static void fft(std::vector<Complex> &x);

  /**
   * @brief In-place radix-2 DIT IFFT.
   */
  static void ifft(std::vector<Complex> &x);

private:
  static void bitReverse(std::vector<Complex> &x);
};

} // namespace aerotrack
