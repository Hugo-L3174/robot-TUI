#pragma once

#include <string>
#include <vector>

struct RobotConfig
{
  std::string name;
  int actuatedJointsNb;
};

struct ControllerConfig
{
  double timestep;
  std::vector<RobotConfig> ctlRobotConfigs;
};

class ControllerInterface
{
public:
  virtual ~ControllerInterface() = default;

  virtual bool runControl() = 0;
  virtual void startControlThread() = 0;
  virtual void stopControlThread() = 0;

private:
};
