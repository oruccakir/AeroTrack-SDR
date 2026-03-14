#include "udp_receiver.hpp"

#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <cmath>

// Graceful shutdown
static volatile bool g_running = true;

void signalHandler(int signum) {
    (void)signum;
    std::cout << "\n[Ana] Durdurma sinyali alındı, kapatılıyor..." << std::endl;
    g_running = false;
}

void printUsage(const char* prog_name) {
    std::cout << "Kullanım: " << prog_name << " [seçenekler]\n"
              << "\nSeçenekler:\n"
              << "  --port <port>     Dinlenecek UDP portu (varsayılan: 5000)\n"
              << "  --verbose         Her paket için detaylı bilgi yazdır\n"
              << "  --dump <adet>     İlk N paketin I/Q verilerini yazdır\n"
              << "  --help            Bu yardım mesajını göster\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // --- Varsayılan ayarlar ---
    uint16_t listen_port = 5000;
    bool     verbose     = false;
    uint32_t dump_count  = 0;

    // --- Argümanları ayrıştır ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--port" && i + 1 < argc) {
            listen_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--dump" && i + 1 < argc) {
            dump_count = static_cast<uint32_t>(std::stoi(argv[++i]));
        } else {
            std::cerr << "Bilinmeyen argüman: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // --- Başlangıç banner ---
    std::cout << "\n"
              << "╔══════════════════════════════════════════════════════════╗\n"
              << "║         AeroTrack-SDR  Alıcı (ZCU102 Receiver)         ║\n"
              << "╚══════════════════════════════════════════════════════════╝\n"
              << "\n"
              << "  Port          : " << listen_port << "\n"
              << "  Verbose       : " << (verbose ? "Evet" : "Hayır") << "\n"
              << "  I/Q Dump      : " << (dump_count > 0 ? std::to_string(dump_count) + " paket" : "Kapalı") << "\n"
              << std::endl;

    // --- Receiver oluştur ve aç ---
    aerotrack::UdpReceiver receiver(listen_port);

    if (!receiver.open()) {
        std::cerr << "[Ana] UDP soketi açılamadı, çıkılıyor." << std::endl;
        return 1;
    }

    std::cout << "[Ana] Paket bekleniyor... (Durdurmak için Ctrl+C)\n" << std::endl;

    // --- Ana alım döngüsü ---
    auto start_time      = std::chrono::steady_clock::now();
    auto last_status_time = start_time;

    uint32_t last_cpi_index    = 0;
    uint32_t cpi_pulse_count   = 0;
    uint32_t completed_cpis    = 0;
    uint32_t dumped_packets    = 0;

    while (g_running) {
        auto packet = receiver.receivePacket(500);  // 500ms timeout

        if (!packet.valid) {
            continue;
        }

        const auto& hdr = packet.header;

        // CPI takibi
        if (hdr.cpi_index != last_cpi_index) {
            last_cpi_index  = hdr.cpi_index;
            cpi_pulse_count = 0;
        }
        cpi_pulse_count++;

        if (hdr.flags & aerotrack::PacketHeader::FLAG_CPI_END) {
            completed_cpis++;
        }

        // Verbose mod: her paketi yazdır
        if (verbose) {
            std::cout << "[RX] PKT#" << hdr.packet_id
                      << " | CPI:" << hdr.cpi_index
                      << " Pulse:" << hdr.pulse_index
                      << "/" << (cpi_pulse_count)
                      << " | Samples:" << hdr.sample_count
                      << " | Flags: "
                      << ((hdr.flags & aerotrack::PacketHeader::FLAG_CPI_START) ? "START " : "")
                      << ((hdr.flags & aerotrack::PacketHeader::FLAG_CPI_END)   ? "END"    : "")
                      << std::endl;
        }

        // I/Q veri dökümü
        if (dump_count > 0 && dumped_packets < dump_count) {
            std::cout << "\n--- Paket #" << hdr.packet_id
                      << " I/Q Verileri (ilk 8 örnek) ---\n";

            size_t show_count = std::min(packet.samples.size(), static_cast<size_t>(8));
            for (size_t i = 0; i < show_count; ++i) {
                float I = packet.samples[i].real();
                float Q = packet.samples[i].imag();
                float mag = std::sqrt(I * I + Q * Q);
                std::cout << "  [" << std::setw(3) << i << "] "
                          << "I=" << std::fixed << std::setprecision(6) << std::setw(10) << I
                          << "  Q=" << std::setw(10) << Q
                          << "  |A|=" << std::setw(10) << mag
                          << std::endl;
            }
            if (packet.samples.size() > 8) {
                std::cout << "  ... (toplam " << packet.samples.size() << " örnek)\n";
            }
            dumped_packets++;
        }

        // Her 2 saniyede bir durum yazdır
        auto now = std::chrono::steady_clock::now();
        auto since_status = std::chrono::duration<double>(now - last_status_time);

        if (since_status.count() >= 2.0 && !verbose) {
            auto total_elapsed = std::chrono::duration<double>(now - start_time);

            std::cout << "\r[Durum] Paket: " << receiver.getTotalPacketsReceived()
                      << " | CPI: " << completed_cpis
                      << " | Süre: " << std::fixed << std::setprecision(1)
                      << total_elapsed.count() << "s"
                      << " | Hız: " << std::setprecision(2)
                      << receiver.getTotalBytesReceived() / total_elapsed.count() / 1e6
                      << " MB/s"
                      << " | Hatalı: " << receiver.getInvalidPackets()
                      << std::flush;

            last_status_time = now;
        }
    }

    // --- Kapanış ---
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration<double>(end_time - start_time);

    std::cout << "\n\n"
              << "╔══════════════════════════════════════════════════════════╗\n"
              << "║                      Alım Özeti                        ║\n"
              << "╚══════════════════════════════════════════════════════════╝\n"
              << "  Toplam Paket             : " << receiver.getTotalPacketsReceived() << "\n"
              << "  Tamamlanan CPI           : " << completed_cpis << "\n"
              << "  Toplam Veri              : " << std::fixed << std::setprecision(2)
              << receiver.getTotalBytesReceived() / (1024.0 * 1024.0) << " MB\n"
              << "  Hatalı Paket             : " << receiver.getInvalidPackets() << "\n"
              << "  Toplam Süre              : " << total_duration.count() << " s\n"
              << "  Ortalama Veri Hızı       : "
              << receiver.getTotalBytesReceived() / total_duration.count() / 1e6
              << " MB/s\n"
              << std::endl;

    receiver.close();

    return 0;
}
