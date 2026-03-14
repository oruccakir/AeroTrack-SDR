#include "udp_receiver.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace aerotrack {

UdpReceiver::UdpReceiver(uint16_t listen_port)
    : listen_port_(listen_port)
    , socket_fd_(-1)
{
}

UdpReceiver::~UdpReceiver()
{
    close();
}

bool UdpReceiver::open()
{
    // UDP soket oluştur
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "[UDP-RX] Soket oluşturma hatası: "
                  << strerror(errno) << std::endl;
        return false;
    }

    // SO_REUSEADDR ayarla
    int reuse = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Alım buffer boyutunu artır
    int rcvbuf_size = 4 * 1024 * 1024;  // 4 MB
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF,
                   &rcvbuf_size, sizeof(rcvbuf_size)) < 0) {
        std::cerr << "[UDP-RX] UYARI: Alım buffer boyutu ayarlanamadı: "
                  << strerror(errno) << std::endl;
    }

    // Bind
    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(listen_port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr),
             sizeof(addr)) < 0) {
        std::cerr << "[UDP-RX] Bind hatası (port " << listen_port_ << "): "
                  << strerror(errno) << std::endl;
        ::close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    std::cout << "[UDP-RX] Dinleniyor → port " << listen_port_ << std::endl;
    return true;
}

void UdpReceiver::close()
{
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        std::cout << "[UDP-RX] Soket kapatıldı. Toplam alınan: "
                  << total_packets_received_ << " paket, "
                  << total_bytes_received_ / 1024 << " KB" << std::endl;
    }
}

UdpReceiver::ParsedPacket UdpReceiver::receivePacket(int timeout_ms)
{
    ParsedPacket result;
    result.valid = false;

    if (socket_fd_ < 0) {
        std::cerr << "[UDP-RX] HATA: Soket açık değil!" << std::endl;
        return result;
    }

    // Timeout kontrolü (poll kullanarak)
    if (timeout_ms > 0) {
        struct pollfd pfd;
        pfd.fd     = socket_fd_;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, timeout_ms);
        if (ret == 0) {
            // Timeout
            return result;
        } else if (ret < 0) {
            if (errno != EINTR) {
                std::cerr << "[UDP-RX] Poll hatası: "
                          << strerror(errno) << std::endl;
            }
            return result;
        }
    }

    // Paket al
    uint8_t buffer[MAX_PACKET_SIZE];
    struct sockaddr_in sender_addr{};
    socklen_t sender_len = sizeof(sender_addr);

    ssize_t bytes_received = recvfrom(
        socket_fd_,
        buffer,
        MAX_PACKET_SIZE,
        0,
        reinterpret_cast<struct sockaddr*>(&sender_addr),
        &sender_len
    );

    if (bytes_received < 0) {
        if (errno != EINTR) {
            std::cerr << "[UDP-RX] Alım hatası: "
                      << strerror(errno) << std::endl;
        }
        return result;
    }

    // Minimum boyut kontrolü (en az header kadar olmalı)
    if (static_cast<size_t>(bytes_received) < sizeof(PacketHeader)) {
        std::cerr << "[UDP-RX] Paket çok küçük: " << bytes_received
                  << " byte" << std::endl;
        invalid_packets_++;
        return result;
    }

    // Header'ı parse et
    std::memcpy(&result.header, buffer, sizeof(PacketHeader));

    // Sync word kontrolü
    if (result.header.sync_word != 0xAE50AE50) {
        std::cerr << "[UDP-RX] Geçersiz sync word: 0x" << std::hex
                  << result.header.sync_word << std::dec << std::endl;
        invalid_packets_++;
        return result;
    }

    // I/Q verisi boyut kontrolü
    size_t expected_data_size = result.header.sample_count * sizeof(IQSample);
    size_t actual_data_size   = bytes_received - sizeof(PacketHeader);

    if (actual_data_size < expected_data_size) {
        std::cerr << "[UDP-RX] Eksik I/Q verisi: beklenen "
                  << expected_data_size << ", alınan "
                  << actual_data_size << std::endl;
        invalid_packets_++;
        return result;
    }

    // I/Q örneklerini çıkar
    result.samples.resize(result.header.sample_count);
    std::memcpy(result.samples.data(),
                buffer + sizeof(PacketHeader),
                expected_data_size);

    result.valid = true;
    total_packets_received_++;
    total_bytes_received_ += bytes_received;

    return result;
}

} // namespace aerotrack
