#pragma once

#include <string>
#include <vector>

struct ServoInfo
{
  struct gains
  {
    double kp = 0.;
    double kd = 0.;
    double ki = 0.;
  };

  struct targets
  {
    // goals in rad
    double pos = 0;
    double vel = 0;
    double acc = 0;
  };

  int id = 0;

  double position = 0.;
  double velocity = 0.;
  double voltage = 0.;
  double current = 0.;
  double torque = 0.;
  double temperature = 0.;
  bool torqueEnabled = false;

  gains servoGains;
  targets servoTargets;
};

class ServoHardware
{
public:
  virtual ~ServoHardware() = default;

  virtual bool openLink(const std::string & device) = 0;
  virtual bool closeLink() = 0;
  virtual std::vector<int> detectServos() = 0;

  virtual ServoInfo getServoData(const int & id) = 0;
  virtual bool servoOn(const int & id, const bool & activate) = 0;
  virtual bool setServoGains(const int & id, const double & P, const double & D, const double & I) = 0;
  virtual bool setServoTarget(const int & id, const ServoInfo::targets & target) = 0;
  virtual bool setServoTargets(const std::vector<int> & ids, std::vector<ServoInfo::targets> targets) = 0;

private:
};
