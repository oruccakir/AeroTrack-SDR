#pragma once

#include <complex>
#include <cstdint>
#include <cstring>
#include <vector>

namespace aerotrack {

/**
 * @brief Transmitter ile aynı paket başlık yapısı.
 * Toplam: 24 byte.
 */
struct __attribute__((packed)) PacketHeader {
  uint32_t sync_word = 0xAE50AE50;
  uint32_t packet_id = 0;
  uint32_t pulse_index = 0;
  uint32_t sample_count = 0;
  uint32_t cpi_index = 0;
  uint32_t flags = 0;

  static constexpr uint32_t FLAG_CPI_START = 0x01;
  static constexpr uint32_t FLAG_CPI_END = 0x02;
};

static_assert(sizeof(PacketHeader) == 24,
              "PacketHeader must be exactly 24 bytes");

/**
 * @brief UDP soket üzerinden paket alıcı.
 *
 * Belirtilen portta dinler, gelen paketleri ayrıştırır
 * ve I/Q verilerini çıkarır.
 */
class UdpReceiver {
public:
  using IQSample = std::complex<float>;
  using IQVector = std::vector<IQSample>;

  /**
   * @brief Ayrıştırılmış paket yapısı.
   */
  struct ParsedPacket {
    PacketHeader header;
    IQVector samples;
    bool valid = false;
  };

  /**
   * @param listen_port Dinlenecek UDP port numarası
   */
  explicit UdpReceiver(uint16_t listen_port);
  ~UdpReceiver();

  // Non-copyable
  UdpReceiver(const UdpReceiver &) = delete;
  UdpReceiver &operator=(const UdpReceiver &) = delete;

  /**
   * @brief Soketi açar ve bind eder.
   * @return true başarılıysa
   */
  bool open();

  /**
   * @brief Soketi kapatır.
   */
  void close();

  /**
   * @brief Bir paket alır ve ayrıştırır.
   * @param timeout_ms Zaman aşımı (milisaniye), 0 = bloklayıcı
   * @return Ayrıştırılmış paket
   */
  ParsedPacket receivePacket(int timeout_ms = 1000);

  bool isOpen() const { return socket_fd_ >= 0; }

  uint64_t getTotalPacketsReceived() const { return total_packets_received_; }
  uint64_t getTotalBytesReceived() const { return total_bytes_received_; }
  uint64_t getInvalidPackets() const { return invalid_packets_; }

private:
  uint16_t listen_port_;
  int socket_fd_;

  uint64_t total_packets_received_ = 0;
  uint64_t total_bytes_received_ = 0;
  uint64_t invalid_packets_ = 0;

  // Maksimum paket boyutu: header(24) + 1024 sample * 8 byte
  static constexpr size_t MAX_PACKET_SIZE = 24 + 1024 * 8;
};

} // namespace aerotrack
