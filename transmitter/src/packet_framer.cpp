#include "packet_framer.hpp"
#include <cstring>

namespace aerotrack {

PacketFramer::PacketFramer()
    : packet_counter_(0)
{
}

std::vector<uint8_t> PacketFramer::framePacket(
    const IQVector& samples,
    uint32_t pulse_index,
    uint32_t cpi_index,
    uint32_t num_pulses_per_cpi)
{
    // Başlık oluştur
    PacketHeader header;
    header.sync_word    = 0xAE50AE50;
    header.packet_id    = packet_counter_++;
    header.pulse_index  = pulse_index;
    header.sample_count = static_cast<uint32_t>(samples.size());
    header.cpi_index    = cpi_index;
    header.flags        = 0;

    // CPI başlangıç/bitiş flag'leri
    if (pulse_index == 0) {
        header.flags |= PacketHeader::FLAG_CPI_START;
    }
    if (pulse_index == num_pulses_per_cpi - 1) {
        header.flags |= PacketHeader::FLAG_CPI_END;
    }

    // Toplam paket boyutu: header + I/Q verileri
    // Her I/Q örneği = 2 × float (8 byte)
    size_t data_size = samples.size() * sizeof(IQSample);
    size_t total_size = sizeof(PacketHeader) + data_size;

    std::vector<uint8_t> buffer(total_size);

    // Başlığı kopyala
    std::memcpy(buffer.data(), &header, sizeof(PacketHeader));

    // I/Q verilerini kopyala
    std::memcpy(buffer.data() + sizeof(PacketHeader),
                samples.data(),
                data_size);

    return buffer;
}

} // namespace aerotrack
