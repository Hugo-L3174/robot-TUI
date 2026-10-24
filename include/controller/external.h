#pragma once
#include "connection/UDPServer.h"
#include "connection/external_connection.h"
#include "controllers.h"
#include <memory>
#include <string>

struct ExternalControllerConfig : ControllerConfig
{
  ExternalConnectionType connectionType = ExternalConnectionType::UDP;
  // Default is 0.0.0.0, means listening to all
  std::string bindAddress = "0.0.0.0";
  unsigned short port = 9000;
};

/**
 * @brief The ExternalController implementation assumes that a controller is running somewhere else and sending the
 * resulting commands. This class implements a server to receive them and transmit to the robot.
 *
 */
class ExternalController : public ControllerInterface
{
public:
  explicit ExternalController(const ExternalControllerConfig & config, std::shared_ptr<spdlog::logger> logger = {});
  // The external controller run function runs the chosen communication interface, sends the robot sensor data to the
  // client controller and gets the commands from the client controller
  bool runControl(std::mutex & stateMutex,
                  const std::vector<ServoInfo> & states,
                  std::mutex & commandMutex,
                  std::vector<ServoInfo> & commands) override;
  bool startControl() override;
  bool stopControl() override;

private:
  ExternalControllerConfig config_;
  std::unique_ptr<UDPServer> udpServer_;
};
