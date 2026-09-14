#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <serial.h>

#include "servos/servo_factory.h"

#include "controller_factory.h"

enum class ConnectionType
{
  Serial,
};

class Interface
{
public:
  Interface(/*hardware& hardware,
              controller& controller*/);

  // Should set connected_ var
  void connect(const std::string & device);
  // Should clear servos_ and set connected_ to false
  void disconnect();

  void startController();
  void stopController();

  bool connected() const
  {
    return connected_;
  }
  bool controllerRunning() const;

  void detectSerialPorts();
  const std::vector<std::string> & getSerialPorts() const
  {
    return serialPorts_;
  }

  void detectServos();
  const std::vector<int> & getServoIds() const
  {
    return servoIds_;
  }
  ServoInfo getServoData(int id);

  // Factory to build the correct type of ServoHardware
  void setupServos(MotorType type);

  // Factory to build the correct type of Controller
  void setupController(ControllerType type);

  bool setServoTarget(int id, double targetPos, double targetVel = 0., double targetAcc = 0.);

  bool setServoGains(int id, double P, double D, double I);

  // @todo this is a single value now but might want to have a vector
  // and servos would be a map with the connection link or robot as key?
  // In case single controller with different connections (multi-robot)
  ConnectionType connectionType_;

private:
  bool connected_;

  std::unique_ptr<ServoHardware> servoHardware_;
  std::unique_ptr<ControllerInterface> controller_;

  // To be detected at construction
  // May need to be more specific than just the names
  std::vector<std::string> serialPorts_;

  // Servo ids
  std::vector<int> servoIds_;
};
