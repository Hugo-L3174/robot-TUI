#pragma once

#include "servos/servos.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

enum class ServoPacketKind : std::uint8_t
{
  Commands = 1,
  States = 2
};

namespace servo_packet
{
// Arbitrary signature
inline constexpr std::array<char, 4> magic{'S', 'R', 'V', 'O'};
inline constexpr std::uint8_t version = 1;
inline constexpr std::size_t maxDatagramSize = 65507;

struct Header
{
  std::array<char, 4> signature;
  std::uint8_t version;
  ServoPacketKind kind;
  std::uint16_t recordCount;
};

inline constexpr std::size_t headerSize = sizeof(Header);
inline constexpr std::size_t recordSize = sizeof(ServoInfo);
inline constexpr std::size_t maxRecords = (maxDatagramSize - headerSize) / recordSize;

// Encode the vector of data to send into sendable packets with header
inline std::vector<std::uint8_t> encode(ServoPacketKind kind, const std::vector<ServoInfo> & servosData)
{
  if(servosData.size() > maxRecords) return {};

  // Make header with data size
  const Header header{magic, version, kind, static_cast<std::uint16_t>(servosData.size())};
  const std::size_t packetSize = headerSize + servosData.size() * recordSize;
  std::vector<std::uint8_t> packet(packetSize);
  // Put header in packet first
  std::memcpy(packet.data(), &header, headerSize);
  if(!servosData.empty())
  {
    // Put actual data after header
    std::memcpy(packet.data() + headerSize, servosData.data(), servosData.size() * recordSize);
  }
  return packet;
}

// Decode a received packet into a vector of data
inline bool decode(const std::vector<std::uint8_t> & packet,
                   ServoPacketKind expectedKind,
                   std::vector<ServoInfo> & servosData)
{
  if(packet.size() < headerSize) return false;

  Header header{};
  // Get the received packet header
  std::memcpy(&header, packet.data(), headerSize);
  // Check types etc
  if(header.signature != magic || header.version != version || header.kind != expectedKind) return false;
  const std::size_t expectedSize = headerSize + header.recordCount * recordSize;
  if(packet.size() != expectedSize) return false;

  // Create output vector with size info from header
  std::vector<ServoInfo> decoded(header.recordCount);
  if(!decoded.empty())
  {
    // Get packet payload
    std::memcpy(decoded.data(), packet.data() + headerSize, decoded.size() * recordSize);
  }
  servosData = std::move(decoded);
  return true;
}
} // namespace servo_packet
