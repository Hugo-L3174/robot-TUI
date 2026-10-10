#pragma once

#include "connection/servo_packet.h"

#include <string>
#include <vector>

// UDP transport class to receive commands and send states
class UDPServer
{
public:
  UDPServer(std::string bindAddress, unsigned short port);
  ~UDPServer();

  // Start the server and bind the address and port passed at construction
  bool start();
  // Stop server by closing socket
  bool stop();
  bool running() const;

  // Receive data sent by client, decode it and put it in commands buffer
  bool receiveCommands(std::vector<ServoInfo> & commands);
  // Send state data to last connected client
  bool sendStates(const std::vector<ServoInfo> & states);

private:
  std::string bindAddress_;
  unsigned short port_;
  int socket_ = -1;
  // Address of last connected peer to send back state data
  std::vector<std::uint8_t> peerAddress_;
};
