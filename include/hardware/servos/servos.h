#pragma once

#include <string>
#include <vector>

// servo command structure
struct ServoTarget
{
  // @fixme redundant, find a more elegant way not needing it in the target struct directly
  int id = 0;

  double kp = 0.;
  double kd = 0.;
  double ki = 0.;
  double pos = 0.;
  double vel = 0.;
  double torque = 0.;
};

// servo data structure
struct ServoInfo
{
  int id = 0;

  double position = 0.;
  double velocity = 0.;
  double voltage = 0.;
  double current = 0.;
  double torque = 0.;
  double temperature = 0.;
  bool torqueEnabled = false;

  // some servo types hold their targets internally so using the target types in data as well
  ServoTarget servoTargets;
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
  virtual bool setServoTarget(const int & id, const ServoTarget & target) = 0;
  virtual bool setServoTargets(const std::vector<int> & ids, std::vector<ServoTarget> targets) = 0;

private:
};
