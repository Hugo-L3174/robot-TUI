#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
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
  // @todo inline to return if the control thread is running
  bool controllerRunning() const;

  void detectSerialPorts();
  const std::vector<std::string> & getSerialPorts() const
  {
    return serialPorts_;
  }

  // Ping hardware and fill servoIds_ vector with detected servos
  void detectServos();
  const std::vector<int> & getServoIds() const
  {
    return servoIds_;
  }

  // @todo these functions should return the buffered values instead of the direct hardware data
  // Get a specific servo data
  ServoInfo getServoData(int id);
  // Get data from all servos in servoIds_
  std::vector<ServoInfo> getServoData();
  // Update an existing vector with data from all servos
  void getServoData(std::vector<ServoInfo> & dataBuffer);

  // Factory to build the correct type of ServoHardware
  void setupServos(MotorType type);

  // Factory to build the correct type of Controller
  void setupController(ControllerType type,
                       const ControllerConfig & config = ControllerConfig{},
                       std::shared_ptr<spdlog::logger> logger = {});

  void setServoTarget(int id, double targetPos, double targetVel = 0., double targetAcc = 0.);
  void setServoTargets(const std::vector<int> & ids, const std::vector<double> & targetsPos);
  void setServoTargets(const std::vector<int> & ids,
                       const std::vector<double> & targetsPos,
                       const std::vector<double> & targetsVel);
  void setServoTargets(const std::vector<int> & ids,
                       const std::vector<double> & targetsPos,
                       const std::vector<double> & targetsVel,
                       const std::vector<double> & targetsAcc);
  // @todo add another setServo that takes a vector of commands directly

  void setServoGains(int id, double P, double D, double I);

  bool setServoOn(int id, bool activate);

  // @todo this is a single value now but might want to have a vector
  // and servos would be a map with the connection link or robot as key?
  // In case single controller with different connections (multi-robot)
  ConnectionType connectionType_;

private:
  // Hardware link connection state
  bool connected_;

  // Timestep for hardware loop (unit: second)
  double hardwareTimestep_;

  std::unique_ptr<ServoHardware> servoHardware_;
  std::unique_ptr<ControllerInterface> controller_;

  // Servo commands vector, updated by the controller and sent to the hardware
  std::vector<ServoInfo> controllerCommands_;
  // Servo states vector, updated by the hardware and sent as feedback to the controller
  std::vector<ServoInfo> motorStates_;
  // Thread protection for commands and states
  mutable std::mutex commandsMutex_;
  mutable std::mutex statesMutex_;

  // Thread polling the connected hardware to update state data and send commands
  std::jthread hardwareThread_;
  // Bool flag to send or not commands in the hardware thread
  std::atomic<bool> commanding_;

  // Thread running the controller specific control function at the specified frequency
  std::jthread controllerThread_;

  // To be detected at construction
  // May need to be more specific than just the names
  std::vector<std::string> serialPorts_;

  // Servo ids
  std::vector<int> servoIds_;
};
