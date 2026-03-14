#include "udp_transmitter.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cerrno>

namespace aerotrack {

UdpTransmitter::UdpTransmitter(const std::string& target_ip, uint16_t target_port)
    : target_ip_(target_ip)
    , target_port_(target_port)
    , socket_fd_(-1)
{
}

UdpTransmitter::~UdpTransmitter()
{
    close();
}

bool UdpTransmitter::open()
{
    // UDP soket oluştur
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "[UDP] Soket oluşturma hatası: "
                  << strerror(errno) << std::endl;
        return false;
    }

    // Gönderim buffer boyutunu artır (yüksek veri hızı için)
    int sndbuf_size = 4 * 1024 * 1024;  // 4 MB
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF,
                   &sndbuf_size, sizeof(sndbuf_size)) < 0) {
        std::cerr << "[UDP] UYARI: Gönderim buffer boyutu ayarlanamadı: "
                  << strerror(errno) << std::endl;
    }

    std::cout << "[UDP] Soket açıldı → " << target_ip_
              << ":" << target_port_ << std::endl;

    return true;
}

void UdpTransmitter::close()
{
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        std::cout << "[UDP] Soket kapatıldı. Toplam gönderilen: "
                  << total_packets_sent_ << " paket, "
                  << total_bytes_sent_ / 1024 << " KB" << std::endl;
    }
}

ssize_t UdpTransmitter::send(const std::vector<uint8_t>& data)
{
    if (socket_fd_ < 0) {
        std::cerr << "[UDP] HATA: Soket açık değil!" << std::endl;
        return -1;
    }

    // Hedef adres yapısı
    struct sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(target_port_);

    if (inet_pton(AF_INET, target_ip_.c_str(), &dest_addr.sin_addr) <= 0) {
        std::cerr << "[UDP] HATA: Geçersiz IP adresi: "
                  << target_ip_ << std::endl;
        failed_packets_++;
        return -1;
    }

    ssize_t bytes_sent = sendto(
        socket_fd_,
        data.data(),
        data.size(),
        0,
        reinterpret_cast<struct sockaddr*>(&dest_addr),
        sizeof(dest_addr)
    );

    if (bytes_sent < 0) {
        std::cerr << "[UDP] Gönderim hatası: " << strerror(errno) << std::endl;
        failed_packets_++;
        return -1;
    }

    total_bytes_sent_ += bytes_sent;
    total_packets_sent_++;

    return bytes_sent;
}

void UdpTransmitter::resetStats()
{
    total_bytes_sent_ = 0;
    total_packets_sent_ = 0;
    failed_packets_ = 0;
}

} // namespace aerotrack
