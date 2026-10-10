#include "connection/UDPServer.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

// Build string ip address + port into a useable socket address
sockaddr_in makeAddress(const std::string & address, unsigned short port)
{
  sockaddr_in result{};
  result.sin_family = AF_INET;
  result.sin_port = htons(port);
  // Convert the string address into network format address
  if(inet_pton(AF_INET, address.c_str(), &result.sin_addr) != 1)
  {
    throw std::invalid_argument("UDP bind address must be an IPv4 address");
  }
  return result;
}

UDPServer::UDPServer(std::string bindAddress, unsigned short port) : bindAddress_(std::move(bindAddress)), port_(port)
{
}

UDPServer::~UDPServer()
{
  stop();
}

bool UDPServer::start()
{
  if(socket_ >= 0) return true;
  const auto address = makeAddress(bindAddress_, port_);
  // Open socket in ipv4 and udp mode
  const int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(fd < 0) return false;

  if(bind(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) < 0)
  {
    close(fd);
    return false;
  }
  socket_ = fd;
  return true;
}

bool UDPServer::stop()
{
  if(socket_ >= 0)
  {
    const bool closed = close(socket_) == 0;
    socket_ = -1;
    peerAddress_.clear();
    return closed;
  }
  peerAddress_.clear();
  return true;
}

bool UDPServer::running() const
{
  return socket_ >= 0;
}

bool UDPServer::receiveCommands(std::vector<ServoInfo> & commands)
{
  if(socket_ < 0) return false;
  sockaddr_in peer{};
  socklen_t peerLength = sizeof(peer);
  std::vector<std::uint8_t> packet(servo_packet::maxDatagramSize);
  // Receive servo packets from socket and keep peer address sending them
  const auto size =
      recvfrom(socket_, packet.data(), packet.size(), MSG_DONTWAIT, reinterpret_cast<sockaddr *>(&peer), &peerLength);
  if(size < 0) return false;
  packet.resize(static_cast<std::size_t>(size));

  std::vector<ServoInfo> decoded;
  // Decode received packet into expected commands format
  if(!servo_packet::decode(packet, ServoPacketKind::Commands, decoded)) return false;
  peerAddress_.resize(sizeof(peer));
  std::memcpy(peerAddress_.data(), &peer, sizeof(peer));
  commands = std::move(decoded);
  return true;
}

bool UDPServer::sendStates(const std::vector<ServoInfo> & states)
{
  if(socket_ < 0 || peerAddress_.size() != sizeof(sockaddr_in)) return false;
  // Encode state values into a sendable packet
  const auto packet = servo_packet::encode(ServoPacketKind::States, states);
  if(packet.empty() && !states.empty()) return false;
  sockaddr_in peer{};
  std::memcpy(&peer, peerAddress_.data(), sizeof(peer));
  // Send packet to last connected client
  return sendto(socket_, packet.data(), packet.size(), 0, reinterpret_cast<const sockaddr *>(&peer), sizeof(peer))
         == static_cast<ssize_t>(packet.size());
}
