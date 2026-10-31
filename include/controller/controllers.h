#pragma once

#include "servos/servos.h"
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/logger.h>

struct RobotConfig
{
  std::string name;
  int actuatedJointsNb;
};

struct ControllerConfig
{
  virtual ~ControllerConfig() = default;
  // controller timestep in seconds
  double timestep = 0.002;
  std::vector<RobotConfig> ctlRobotConfigs;
};

class ControllerInterface
{
public:
  ControllerInterface() = default;
  explicit ControllerInterface(const ControllerConfig & config, std::shared_ptr<spdlog::logger> logger = {})
  : config_(config), logger_(std::move(logger))
  {
  }
  virtual ~ControllerInterface() = default;

  virtual bool runControl(std::mutex & stateMutex,
                          const std::vector<ServoInfo> & states,
                          std::mutex & commandMutex,
                          std::vector<ServoInfo> & commands) = 0;
  virtual bool startControl() = 0;
  virtual bool stopControl() = 0;
  // Controller timestep in seconds
  inline double timestep() const noexcept
  {
    return config_.timestep;
  };

protected:
  ControllerConfig config_;
  // logger for controller and its lower level components
  std::shared_ptr<spdlog::logger> logger_;
};
