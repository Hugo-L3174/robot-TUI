#include "external.h"

ExternalController::ExternalController(const ExternalControllerConfig & config, std::shared_ptr<spdlog::logger> logger)
: ControllerInterface(config, std::move(logger)), config_(config)
{
  if(config_.connectionType == ExternalConnectionType::UDP)
  {
    udpServer_ = std::make_unique<UDPServer>(config_.bindAddress, config_.port, logger_);
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
    if(logger_) logger_->error("Cannot start external controller: no UDP server is configured");
    return false;
  }
  const bool started = udpServer_->start();
  if(logger_)
  {
    if(started)
      logger_->info("External UDP controller started");
    else
      logger_->error("Failed to start external UDP controller");
  }
  return started;
}

bool ExternalController::stopControl()
{
  if(!udpServer_)
  {
    if(logger_) logger_->warn("Cannot stop external controller: no UDP server is configured");
    return false;
  }
  const bool stopped = udpServer_->stop();
  if(logger_)
  {
    if(stopped)
      logger_->info("External UDP controller stopped");
    else
      logger_->error("Failed to stop external UDP controller");
  }
  return stopped;
}
