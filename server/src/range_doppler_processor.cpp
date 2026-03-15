#include "range_doppler_processor.hpp"
#include <cmath>

namespace aerotrack {

void RangeDopplerProcessor::bitReverse(std::vector<Complex> &x) {
  size_t n = x.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1) {
      j ^= bit;
    }
    j ^= bit;
    if (i < j) {
      std::swap(x[i], x[j]);
    }
  }
}

void RangeDopplerProcessor::fft(std::vector<Complex> &x) {
  size_t n = x.size();
  if (n <= 1)
    return;

  bitReverse(x);

  for (size_t len = 2; len <= n; len <<= 1) {
    float angle = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
    Complex wlen(std::cos(angle), std::sin(angle));
    for (size_t i = 0; i < n; i += len) {
      Complex w(1.0f, 0.0f);
      for (size_t j = 0; j < len / 2; ++j) {
        Complex u = x[i + j];
        Complex v = x[i + j + len / 2] * w;
        x[i + j] = u + v;
        x[i + j + len / 2] = u - v;
        w *= wlen;
      }
    }
  }
}

void RangeDopplerProcessor::ifft(std::vector<Complex> &x) {
  for (auto &val : x) {
    val = std::conj(val);
  }
  fft(x);
  float n = static_cast<float>(x.size());
  for (auto &val : x) {
    val = std::conj(val) / n;
  }
}

std::vector<float>
RangeDopplerProcessor::compute(const std::vector<Complex> &cpi_data,
                               uint32_t num_pulses, uint32_t num_samples,
                               const std::vector<Complex> &ref_chirp) {
  // 1. ref_chirp FFT ve conjugate
  std::vector<Complex> ref_fft(ref_chirp.begin(), ref_chirp.end());
  ref_fft.resize(num_samples, Complex(0, 0));
  fft(ref_fft);
  for (auto &v : ref_fft) {
    v = std::conj(v);
  }

  // 2. Range compression: her darbe için FFT -> conjugate chirp ile çarp -> IFFT
  // range_compressed[pulse][sample]
  std::vector<std::vector<Complex>> range_compressed(num_pulses);

  for (uint32_t p = 0; p < num_pulses; ++p) {
    std::vector<Complex> pulse_data(
        cpi_data.begin() + p * num_samples,
        cpi_data.begin() + (p + 1) * num_samples);

    fft(pulse_data);

    for (uint32_t s = 0; s < num_samples; ++s) {
      pulse_data[s] *= ref_fft[s];
    }

    ifft(pulse_data);
    range_compressed[p] = std::move(pulse_data);
  }

  // 3. Doppler processing: her range bin için pulse'lar arası FFT
  std::vector<float> rd_map(num_samples * num_pulses);

  for (uint32_t r = 0; r < num_samples; ++r) {
    std::vector<Complex> doppler_col(num_pulses);
    for (uint32_t p = 0; p < num_pulses; ++p) {
      doppler_col[p] = range_compressed[p][r];
    }

    fft(doppler_col);

    // FFT shift: 0-freq'i ortaya al
    std::vector<Complex> shifted(num_pulses);
    uint32_t half = num_pulses / 2;
    for (uint32_t d = 0; d < num_pulses; ++d) {
      shifted[d] = doppler_col[(d + half) % num_pulses];
    }

    // Magnitude (dB)
    for (uint32_t d = 0; d < num_pulses; ++d) {
      float mag = std::abs(shifted[d]);
      float db = (mag > 1e-12f) ? 20.0f * std::log10(mag) : -120.0f;
      rd_map[r * num_pulses + d] = db;
    }
  }

  return rd_map;
}

} // namespace aerotrack
