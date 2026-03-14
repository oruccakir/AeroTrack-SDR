#pragma once

#include <complex>
#include <vector>
#include <cstdint>
#include <cstring>

namespace aerotrack {

/**
 * @brief UDP paket başlık yapısı.
 *
 * ZCU102 tarafında kolay ayrıştırma için sabit boyutlu,
 * byte-aligned başlık. Toplam: 24 byte.
 *
 * | Field        | Offset | Size | Description               |
 * |--------------|--------|------|---------------------------|
 * | sync_word    | 0      | 4    | 0xAE50AE50 sabit değer    |
 * | packet_id    | 4      | 4    | Sıralı paket numarası     |
 * | pulse_index  | 8      | 4    | CPI içindeki darbe indexi  |
 * | sample_count | 12     | 4    | Paketteki I/Q örnek sayısı |
 * | cpi_index    | 16     | 4    | CPI index numarası        |
 * | flags        | 20     | 4    | Bitfield: CPI start/end   |
 */
struct __attribute__((packed)) PacketHeader {
    uint32_t sync_word    = 0xAE50AE50;
    uint32_t packet_id    = 0;
    uint32_t pulse_index  = 0;
    uint32_t sample_count = 0;
    uint32_t cpi_index    = 0;
    uint32_t flags        = 0;

    // Flag bitleri
    static constexpr uint32_t FLAG_CPI_START = 0x01;
    static constexpr uint32_t FLAG_CPI_END   = 0x02;
};

static_assert(sizeof(PacketHeader) == 24, "PacketHeader must be exactly 24 bytes");

/**
 * @brief I/Q verilerini UDP paketleri halinde çerçeveler.
 *
 * Her paket: [PacketHeader (24B)] + [I/Q Data (N × 8B)]
 * Her I/Q örneği: [float I (4B)] + [float Q (4B)]
 */
class PacketFramer {
public:
    using IQSample = std::complex<float>;
    using IQVector = std::vector<IQSample>;

    PacketFramer();

    /**
     * @brief Bir darbenin I/Q verilerini paketler.
     * @param samples I/Q örnekleri
     * @param pulse_index CPI içindeki darbe indexi
     * @param cpi_index CPI numarası
     * @param num_pulses_per_cpi CPI başına darbe sayısı
     * @return Hazır gönderim buffer'ı (header + data)
     */
    std::vector<uint8_t> framePacket(
        const IQVector& samples,
        uint32_t pulse_index,
        uint32_t cpi_index,
        uint32_t num_pulses_per_cpi
    );

    /**
     * @brief Toplam gönderilen paket sayısını döndürür.
     */
    uint32_t getPacketCount() const { return packet_counter_; }

    /**
     * @brief Paket sayacını sıfırlar.
     */
    void resetCounter() { packet_counter_ = 0; }

private:
    uint32_t packet_counter_;
};

} // namespace aerotrack
