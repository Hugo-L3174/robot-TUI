#include "external.h"

ExternalController::ExternalController(const ExternalControllerConfig & config) : config_(config)
{
  if(config_.connectionType == ExternalConnectionType::UDP)
  {
    udpServer_ = std::make_unique<UDPServer>(config_.bindAddress, config_.port);
    // For the UDP server we want the thread to run continuously, so we set its timestep to zero
    config_.timestep = 0.;
  }
  else
  {
    // @todo extend to other connection types
  }
}

bool ExternalController::runControl(std::mutex & stateMutex,
                                    const std::vector<ServoInfo> & states,
                                    std::mutex & commandMutex,
                                    std::vector<ServoInfo> & commands)
{
  bool ret = true;
  // @todo instead of hardcoding udp make an external connection pointer
  if(!udpServer_->sendStates(states))
  {
    // If sendStates returned false
    ret = false;
  }

  if(!udpServer_->receiveCommands(commands))
  {
    // If receiveCommands returned false the commands were not received and buffer not updated
    ret = false;
  }
  return ret;
}

bool ExternalController::startControl()
{
  if(!udpServer_)
  {
    return false;
  }
  return udpServer_->start();
}

bool ExternalController::stopControl()
{
  if(!udpServer_)
  {
    return false;
  }
  return udpServer_->stop();
}
