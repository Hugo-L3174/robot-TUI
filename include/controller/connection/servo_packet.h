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
inline constexpr std::size_t dataSize = sizeof(ServoInfo);
inline constexpr std::size_t maxData = (maxDatagramSize - headerSize) / dataSize;
inline constexpr std::size_t commandSize = sizeof(ServoTarget);
inline constexpr std::size_t maxCommand = (maxDatagramSize - headerSize) / commandSize;

// Encode the vector of data to send into sendable packets with header
inline std::vector<std::uint8_t> encodeData(const std::vector<ServoInfo> & servosData)
{
  if(servosData.size() > maxData) return {};

  // Make header with data size
  const Header header{magic, version, ServoPacketKind::States, static_cast<std::uint16_t>(servosData.size())};
  const std::size_t packetSize = headerSize + servosData.size() * dataSize;
  std::vector<std::uint8_t> packet(packetSize);
  // Put header in packet first
  std::memcpy(packet.data(), &header, headerSize);
  if(!servosData.empty())
  {
    // Put actual data after header
    std::memcpy(packet.data() + headerSize, servosData.data(), servosData.size() * dataSize);
  }
  return packet;
}

// Encode the vector of commands to send into sendable packets with header
inline std::vector<std::uint8_t> encodeCommand(const std::vector<ServoTarget> & servosCommands)
{
  if(servosCommands.size() > maxCommand) return {};

  // Make header with command size
  const Header header{magic, version, ServoPacketKind::Commands, static_cast<std::uint16_t>(servosCommands.size())};
  const std::size_t packetSize = headerSize + servosCommands.size() * commandSize;
  std::vector<std::uint8_t> packet(packetSize);
  // Put header in packet first
  std::memcpy(packet.data(), &header, headerSize);
  if(!servosCommands.empty())
  {
    // Put actual data after header
    std::memcpy(packet.data() + headerSize, servosCommands.data(), servosCommands.size() * commandSize);
  }
  return packet;
}

// Decode a received packet into a vector of data
inline bool decodeData(const std::vector<std::uint8_t> & packet, std::vector<ServoInfo> & servosData)
{
  if(packet.size() < headerSize) return false;

  Header header{};
  // Get the received packet header
  std::memcpy(&header, packet.data(), headerSize);
  // Check types etc
  if(header.signature != magic || header.version != version || header.kind != ServoPacketKind::States) return false;
  const std::size_t expectedSize = headerSize + header.recordCount * dataSize;
  if(packet.size() != expectedSize) return false;

  // Create output vector with size info from header
  std::vector<ServoInfo> decoded(header.recordCount);
  if(!decoded.empty())
  {
    // Get packet payload
    std::memcpy(decoded.data(), packet.data() + headerSize, decoded.size() * dataSize);
  }
  servosData = std::move(decoded);
  return true;
}

// Decode a received packet into a vector of commands
inline bool decodeCommand(const std::vector<std::uint8_t> & packet, std::vector<ServoTarget> & servoCommands)
{
  if(packet.size() < headerSize) return false;

  Header header{};
  // Get the received packet header
  std::memcpy(&header, packet.data(), headerSize);
  // Check types etc
  if(header.signature != magic || header.version != version || header.kind != ServoPacketKind::Commands) return false;
  const std::size_t expectedSize = headerSize + header.recordCount * commandSize;
  if(packet.size() != expectedSize) return false;

  // Create output vector with size info from header
  std::vector<ServoTarget> decoded(header.recordCount);
  if(!decoded.empty())
  {
    // Get packet payload
    std::memcpy(decoded.data(), packet.data() + headerSize, decoded.size() * commandSize);
  }
  servoCommands = std::move(decoded);
  return true;
}
} // namespace servo_packet
