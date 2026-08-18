#pragma once

#include <string>
#include <vector>

struct ServoInfo
{
  int id = 0;

  double voltage = 0.;
  double current = 0.;

  double kp = 0.;
  double ki = 0.;
  double kd = 0.;

  double position = 0.;
  double torque = 0.;
  double temperature = 0.;
  bool torqueMode = false;
};

class ServoHardware
{
public:
  virtual ~ServoHardware() = default;

  virtual bool openLink(const std::string & device) = 0;
  virtual bool closeLink() = 0;
  virtual std::vector<int> detectServos() = 0;

  virtual ServoInfo getServoData(int id) = 0;

private:
};
