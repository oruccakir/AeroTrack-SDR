#include "simulation_engine.hpp"
#include "noise_generator.hpp"
#include "packet_framer.hpp"
#include "range_doppler_processor.hpp"
#include "signal_generator.hpp"
#include "target_simulator.hpp"
#include "udp_transmitter.hpp"

#include <chrono>
#include <iostream>

namespace aerotrack {

SimulationEngine::SimulationEngine(SimulationData &data) : data_(data) {}

SimulationEngine::~SimulationEngine() { stop(); }

void SimulationEngine::start() {
  if (running_.load())
    return;

  stop_requested_.store(false);
  running_.store(true);

  {
    std::lock_guard<std::mutex> lock(data_.mtx);
    data_.state = SimState::RUNNING;
    data_.current_cpi = 0;
    data_.total_packets = 0;
    data_.elapsed_s = 0.0;
    data_.throughput_mbps = 0.0;
    data_.target_history.clear();
    data_.latest_iq_waveform.clear();
    data_.range_doppler_map.clear();
  }

  thread_ = std::thread(&SimulationEngine::run, this);
}

void SimulationEngine::stop() {
  stop_requested_.store(true);
  if (thread_.joinable()) {
    thread_.join();
  }
  running_.store(false);
}

void SimulationEngine::run() {
  RadarConfig config;
  std::vector<Target> targets;
  uint32_t max_cpi = 0;
  bool udp_enabled = false;

  {
    std::lock_guard<std::mutex> lock(data_.mtx);
    config = data_.config;
    targets = data_.targets;
    max_cpi = data_.max_cpi_count;
    udp_enabled = data_.udp_enabled;
  }

  // Bileşenleri oluştur
  TargetSimulator target_sim(config);
  for (const auto &t : targets) {
    target_sim.addTarget(t);
  }

  NoiseGenerator noise_gen(42);
  PacketFramer framer;
  SignalGenerator sig_gen(config);

  // Opsiyonel UDP
  std::unique_ptr<UdpTransmitter> udp;
  if (udp_enabled) {
    udp = std::make_unique<UdpTransmitter>(config.target_ip, config.target_port);
    if (!udp->open()) {
      std::cerr << "[Engine] UDP soketi açılamadı, UDP devre dışı." << std::endl;
      udp.reset();
    }
  }

  // Referans chirp (range-Doppler matched filter)
  auto ref_chirp = sig_gen.generateChirp();

  auto start_time = std::chrono::steady_clock::now();
  uint32_t cpi_index = 0;

  while (!stop_requested_.load()) {
    // CPI limit
    if (max_cpi > 0 && cpi_index >= max_cpi) {
      break;
    }

    // CPI verisini topla
    std::vector<std::complex<float>> cpi_data;
    cpi_data.reserve(config.num_pulses_per_cpi * config.num_samples_per_pulse);

    std::vector<std::complex<float>> last_pulse_iq;

    for (uint32_t pulse = 0;
         pulse < config.num_pulses_per_cpi && !stop_requested_.load();
         ++pulse) {
      // 1. Darbe sinyali üret
      auto iq_data = target_sim.simulatePulse(pulse);

      // 2. Gürültü ekle
      iq_data = noise_gen.addNoise(iq_data, config.snr_db);

      // CPI matrisine ekle
      cpi_data.insert(cpi_data.end(), iq_data.begin(), iq_data.end());

      // Son darbenin I/Q'sunu sakla
      if (pulse == config.num_pulses_per_cpi - 1) {
        last_pulse_iq = iq_data;
      }

      // 3. Opsiyonel UDP gönderimi
      if (udp) {
        auto packet = framer.framePacket(iq_data, pulse, cpi_index,
                                         config.num_pulses_per_cpi);
        udp->send(packet);
      } else {
        framer.framePacket(iq_data, pulse, cpi_index,
                           config.num_pulses_per_cpi);
      }

      // PRF zamanlamasını koru
      // Simülasyonu hızlandırmak için gerçek zamanlı bekleme yapmıyoruz
      // (web modunda gerçek zamanlı gerek yok)
    }

    if (stop_requested_.load())
      break;

    // CPI tamamlandı - hedef pozisyonlarını güncelle
    double cpi_duration = config.num_pulses_per_cpi * config.pri_s();
    target_sim.updateTargetPositions(cpi_duration);

    // Range-Doppler map hesapla
    auto rd_map = RangeDopplerProcessor::compute(
        cpi_data, config.num_pulses_per_cpi, config.num_samples_per_pulse,
        ref_chirp);

    // Shared state güncelle
    auto now = std::chrono::steady_clock::now();
    double elapsed =
        std::chrono::duration<double>(now - start_time).count();

    uint64_t total_bytes = udp ? udp->getTotalBytesSent()
                               : static_cast<uint64_t>(framer.getPacketCount()) *
                                     (24 + config.num_samples_per_pulse * 8);

    {
      std::lock_guard<std::mutex> lock(data_.mtx);
      data_.current_cpi = cpi_index + 1;
      data_.total_packets = framer.getPacketCount();
      data_.elapsed_s = elapsed;
      data_.throughput_mbps =
          (elapsed > 0) ? (total_bytes / elapsed / 1e6) : 0.0;
      data_.latest_iq_waveform = last_pulse_iq;
      data_.range_doppler_map = std::move(rd_map);
      data_.rd_range_bins = config.num_samples_per_pulse;
      data_.rd_doppler_bins = config.num_pulses_per_cpi;
      data_.addTargetSnapshot(cpi_index, elapsed, target_sim.getTargets());
    }

    cpi_index++;

    // Küçük bir bekleme ekle - CPU'yu boğmamak için
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Simülasyon bitti
  {
    std::lock_guard<std::mutex> lock(data_.mtx);
    data_.state = SimState::STOPPED;
  }

  if (udp) {
    udp->close();
  }

  running_.store(false);
}

} // namespace aerotrack
