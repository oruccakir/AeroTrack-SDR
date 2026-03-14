#include "noise_generator.hpp"
#include "packet_framer.hpp"
#include "radar_config.hpp"
#include "target_simulator.hpp"
#include "udp_transmitter.hpp"

#include <chrono>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

// Graceful shutdown
static volatile bool g_running = true;

void signalHandler(int signum) {
  (void)signum;
  std::cout << "\n[Ana] Durdurma sinyali alındı, kapatılıyor..." << std::endl;
  g_running = false;
}

/**
 * @brief Kullanım bilgilerini yazdırır.
 */
void printUsage(const char *prog_name) {
  std::cout
      << "Kullanım: " << prog_name << " [seçenekler]\n"
      << "\nSeçenekler:\n"
      << "  --ip <adres>         Hedef IP adresi (varsayılan: 192.168.1.10)\n"
      << "  --port <port>        Hedef UDP portu (varsayılan: 5000)\n"
      << "  --range <metre>      Hedef menzili (varsayılan: 5000 m)\n"
      << "  --velocity <km/h>    Hedef hızı (varsayılan: 300 km/h)\n"
      << "  --rcs <m²>           Radar kesit alanı (varsayılan: 10 m²)\n"
      << "  --snr <dB>           Sinyal-Gürültü Oranı (varsayılan: 20 dB)\n"
      << "  --prf <Hz>           Darbe Tekrarlama Frekansı (varsayılan: 1000 "
         "Hz)\n"
      << "  --pulses <adet>      CPI başına darbe sayısı (varsayılan: 64)\n"
      << "  --samples <adet>     Darbe başına örnek sayısı (varsayılan: 256)\n"
      << "  --cpi-count <adet>   Gönderilecek CPI sayısı, 0=sonsuz "
         "(varsayılan: 0)\n"
      << "  --help               Bu yardım mesajını göster\n"
      << std::endl;
}

/**
 * @brief Radar konfigürasyon özetini yazdırır.
 */
void printConfig(const aerotrack::RadarConfig &config) {
  std::cout
      << "\n"
      << "╔══════════════════════════════════════════════════════════╗\n"
      << "║          AeroTrack-SDR  Sentetik Veri Üreteci           ║\n"
      << "╚══════════════════════════════════════════════════════════╝\n"
      << "\n"
      << "  RF Parametreleri:\n"
      << "    Taşıyıcı Frekans     : " << config.carrier_frequency_hz / 1e9
      << " GHz\n"
      << "    Bant Genişliği       : " << config.bandwidth_hz / 1e6 << " MHz\n"
      << "    Darbe Süresi          : " << config.pulse_duration_s * 1e6
      << " µs\n"
      << "    Dalga Boyu            : " << config.wavelength() * 100 << " cm\n"
      << "\n"
      << "  Zamanlama:\n"
      << "    PRF                   : " << config.prf_hz << " Hz\n"
      << "    PRI                   : " << config.pri_s() * 1e3 << " ms\n"
      << "    Darbe/CPI             : " << config.num_pulses_per_cpi << "\n"
      << "    Örnek/Darbe           : " << config.num_samples_per_pulse << "\n"
      << "\n"
      << "  Performans Sınırları:\n"
      << "    Max Belirsiz Menzil   : "
      << config.max_unambiguous_range() / 1000.0 << " km\n"
      << "    Max Belirsiz Hız      : "
      << config.max_unambiguous_velocity() * 3.6 << " km/h\n"
      << "    Menzil Çözünürlüğü    : " << config.range_resolution() << " m\n"
      << "    Hız Çözünürlüğü       : " << config.velocity_resolution() * 3.6
      << " km/h\n"
      << "\n"
      << "  Ağ:\n"
      << "    Hedef                 : " << config.target_ip << ":"
      << config.target_port << "\n"
      << "    Paket Boyutu          : "
      << (24 + config.num_samples_per_pulse * 8) << " byte\n"
      << "    Veri Hızı (tahmini)   : " << std::fixed << std::setprecision(2)
      << (24 + config.num_samples_per_pulse * 8) * config.prf_hz / 1e6
      << " MB/s\n"
      << "\n"
      << "  SNR                     : " << config.snr_db << " dB\n"
      << std::endl;
}

int main(int argc, char *argv[]) {
  // SIGINT (Ctrl+C) handler
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  // --- Varsayılan konfigürasyon ---
  aerotrack::RadarConfig config;
  double target_range = 5000.0;   // metre
  double target_velocity = 300.0; // km/h
  double target_rcs = 10.0;       // m²
  uint32_t max_cpi_count = 0;     // 0 = sonsuz

  // --- Komut satırı argümanlarını ayrıştır ---
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (arg == "--ip" && i + 1 < argc) {
      config.target_ip = argv[++i];
    } else if (arg == "--port" && i + 1 < argc) {
      config.target_port = static_cast<uint16_t>(std::stoi(argv[++i]));
    } else if (arg == "--range" && i + 1 < argc) {
      target_range = std::stod(argv[++i]);
    } else if (arg == "--velocity" && i + 1 < argc) {
      target_velocity = std::stod(argv[++i]);
    } else if (arg == "--rcs" && i + 1 < argc) {
      target_rcs = std::stod(argv[++i]);
    } else if (arg == "--snr" && i + 1 < argc) {
      config.snr_db = std::stod(argv[++i]);
    } else if (arg == "--prf" && i + 1 < argc) {
      config.prf_hz = std::stod(argv[++i]);
    } else if (arg == "--pulses" && i + 1 < argc) {
      config.num_pulses_per_cpi = static_cast<uint32_t>(std::stoi(argv[++i]));
    } else if (arg == "--samples" && i + 1 < argc) {
      config.num_samples_per_pulse =
          static_cast<uint32_t>(std::stoi(argv[++i]));
    } else if (arg == "--cpi-count" && i + 1 < argc) {
      max_cpi_count = static_cast<uint32_t>(std::stoi(argv[++i]));
    } else {
      std::cerr << "Bilinmeyen argüman: " << arg << std::endl;
      printUsage(argv[0]);
      return 1;
    }
  }

  // --- Konfigürasyon özetini yazdır ---
  printConfig(config);

  // --- Bileşenleri oluştur ---
  aerotrack::TargetSimulator target_sim(config);
  aerotrack::NoiseGenerator noise_gen(42);
  aerotrack::PacketFramer framer;
  aerotrack::UdpTransmitter udp(config.target_ip, config.target_port);

  // Hedef ekle
  target_sim.addTarget(aerotrack::Target(target_range, target_velocity,
                                         target_rcs, 0.0, "SimHedef-1"));

  // UDP soketi aç
  if (!udp.open()) {
    std::cerr << "[Ana] UDP soketi açılamadı, çıkılıyor." << std::endl;
    return 1;
  }

  // --- Ana gönderim döngüsü ---
  std::cout << "[Ana] Veri gönderimi başlıyor... (Durdurmak için Ctrl+C)\n"
            << std::endl;

  uint32_t cpi_index = 0;
  auto start_time = std::chrono::steady_clock::now();
  auto last_status_time = start_time;

  while (g_running) {
    // CPI sayı limiti kontrolü
    if (max_cpi_count > 0 && cpi_index >= max_cpi_count) {
      std::cout << "\n[Ana] " << max_cpi_count
                << " CPI gönderildi, durduruluyor." << std::endl;
      break;
    }

    // Bir CPI'daki tüm darbeleri üret ve gönder
    for (uint32_t pulse = 0; pulse < config.num_pulses_per_cpi && g_running;
         ++pulse) {
      auto pulse_start = std::chrono::steady_clock::now();

      // 1. Darbe sinyali üret (tüm hedeflerden gelen ekolar)
      auto iq_data = target_sim.simulatePulse(pulse);

      // 2. Gürültü ekle
      iq_data = noise_gen.addNoise(iq_data, config.snr_db);

      // 3. Paketi çerçevele
      auto packet = framer.framePacket(iq_data, pulse, cpi_index,
                                       config.num_pulses_per_cpi);

      // 4. UDP üzerinden gönder
      udp.send(packet);

      // PRF zamanlamasını koru
      auto pulse_end = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration<double>(pulse_end - pulse_start);
      double target_period = config.pri_s();

      if (elapsed.count() < target_period) {
        auto sleep_time =
            std::chrono::duration<double>(target_period - elapsed.count());
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int64_t>(sleep_time.count() * 1e6)));
      }
    }

    // CPI tamamlandı → hedef pozisyonlarını güncelle
    double cpi_duration = config.num_pulses_per_cpi * config.pri_s();
    target_sim.updateTargetPositions(cpi_duration);

    cpi_index++;

    // Her 5 CPI'da bir durum yazdır
    auto now = std::chrono::steady_clock::now();
    auto since_status = std::chrono::duration<double>(now - last_status_time);

    if (since_status.count() >= 2.0) {
      auto total_elapsed = std::chrono::duration<double>(now - start_time);
      auto &targets = target_sim.getTargets();

      std::cout << "\r[Durum] CPI: " << cpi_index
                << " | Paket: " << framer.getPacketCount()
                << " | Süre: " << std::fixed << std::setprecision(1)
                << total_elapsed.count() << "s"
                << " | Hız: " << std::setprecision(2)
                << udp.getTotalBytesSent() / total_elapsed.count() / 1e6
                << " MB/s";

      if (!targets.empty()) {
        std::cout << " | Hedef: " << std::setprecision(0) << targets[0].range_m
                  << "m, " << targets[0].velocity_kmh << "km/h";
      }

      std::cout << std::flush;
      last_status_time = now;
    }
  }

  // --- Kapanış ---
  auto end_time = std::chrono::steady_clock::now();
  auto total_duration = std::chrono::duration<double>(end_time - start_time);

  std::cout << "\n\n"
            << "╔══════════════════════════════════════════════════════════╗\n"
            << "║                    Gönderim Özeti                       ║\n"
            << "╚══════════════════════════════════════════════════════════╝\n"
            << "  Toplam CPI              : " << cpi_index << "\n"
            << "  Toplam Paket            : " << udp.getTotalPacketsSent()
            << "\n"
            << "  Toplam Veri             : " << std::fixed
            << std::setprecision(2)
            << udp.getTotalBytesSent() / (1024.0 * 1024.0) << " MB\n"
            << "  Başarısız Paket         : " << udp.getFailedPackets() << "\n"
            << "  Toplam Süre             : " << total_duration.count()
            << " s\n"
            << "  Ortalama Veri Hızı      : "
            << udp.getTotalBytesSent() / total_duration.count() / 1e6
            << " MB/s\n"
            << std::endl;

  udp.close();

  return 0;
}
