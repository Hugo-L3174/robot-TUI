#pragma once

#include <string>
#include <vector>

// #include "hardware.h"
// #include "controller.h"

enum class ConnectionType
{
  Serial,
};

struct ServoInfo
{
  int id = 0;

  double voltage = 0.;
  double current = 0.;

  double kp = 0.;
  double ki = 0.;
  double kd = 0.;

  double position = 0.;
  bool torque = false;
};

class Interface
{
public:
  Interface(/*hardware& hardware,
              controller& controller*/);

  // Should set connected_ var
  bool connect(const std::string & device);
  // Should clear servos_ and set connected_ to false
  void disconnect();

  void startController();
  void stopController();

  bool connected() const;
  bool controllerRunning() const;

  void detectSerialPorts();
  const std::vector<std::string> & getSerialPorts() const
  {
    return serialPorts_;
  }

  // Should fill servos list
  void detectServos();
  const std::vector<ServoInfo> & getServos() const
  {
    return servos_;
  }

  // @todo this is a single value now but might want to have a vector
  // and servos would be a map with the connection link or robot as key?
  // In case single controller with different connections (multi-robot)
  ConnectionType connectionType_;

private:
  // hardware& hardware_;
  // controller& controller_;
  bool connected_;

  // To be detected at construction
  // May need to be more specific than just the names
  std::vector<std::string> serialPorts_;

  // Servo list
  std::vector<ServoInfo> servos_;
};
