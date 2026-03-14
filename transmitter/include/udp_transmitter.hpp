#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <atomic>

namespace aerotrack {

/**
 * @brief UDP soket üzerinden paket gönderici.
 *
 * POSIX UDP soket oluşturur ve çerçevelenmiş paketleri
 * hedef adrese gönderir.
 */
class UdpTransmitter {
public:
    /**
     * @param target_ip  Hedef IP adresi (örn: "192.168.1.10")
     * @param target_port Hedef UDP port numarası
     */
    UdpTransmitter(const std::string& target_ip, uint16_t target_port);
    ~UdpTransmitter();

    // Non-copyable
    UdpTransmitter(const UdpTransmitter&) = delete;
    UdpTransmitter& operator=(const UdpTransmitter&) = delete;

    /**
     * @brief Soketi açar ve yapılandırır.
     * @return true başarılıysa
     */
    bool open();

    /**
     * @brief Soketi kapatır.
     */
    void close();

    /**
     * @brief Hazır buffer'ı UDP üzerinden gönderir.
     * @param data Gönderilecek veri
     * @return Gönderilen byte sayısı, hata durumunda -1
     */
    ssize_t send(const std::vector<uint8_t>& data);

    /**
     * @brief Soketin açık olup olmadığını kontrol eder.
     */
    bool isOpen() const { return socket_fd_ >= 0; }

    /**
     * @brief İstatistikleri döndürür.
     */
    uint64_t getTotalBytesSent()   const { return total_bytes_sent_; }
    uint64_t getTotalPacketsSent() const { return total_packets_sent_; }
    uint64_t getFailedPackets()    const { return failed_packets_; }

    /**
     * @brief İstatistikleri sıfırlar.
     */
    void resetStats();

private:
    std::string target_ip_;
    uint16_t    target_port_;
    int         socket_fd_;

    // İstatistikler
    std::atomic<uint64_t> total_bytes_sent_{0};
    std::atomic<uint64_t> total_packets_sent_{0};
    std::atomic<uint64_t> failed_packets_{0};
};

} // namespace aerotrack
