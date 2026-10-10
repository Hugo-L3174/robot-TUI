#pragma once

#include "servos/servos.h"
#include <mutex>
#include <string>
#include <vector>

struct RobotConfig
{
  std::string name;
  int actuatedJointsNb;
};

struct ControllerConfig
{
  virtual ~ControllerConfig() = default;
  double timestep = 0.01;
  std::vector<RobotConfig> ctlRobotConfigs;
};

class ControllerInterface
{
public:
  virtual ~ControllerInterface() = default;

  virtual bool runControl(std::mutex & stateMutex,
                          const std::vector<ServoInfo> & states,
                          std::mutex & commandMutex,
                          std::vector<ServoInfo> & commands) = 0;
  virtual bool startControl() = 0;
  virtual bool stopControl() = 0;
  inline double timestep() const noexcept
  {
    return config_.timestep;
  };

protected:
  ControllerConfig config_;
};
