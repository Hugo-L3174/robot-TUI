#include "interface.h"

// temp include for testing
#include <QSerialPortInfo>

Interface::Interface() : connected_(false) {}

void Interface::connect(const std::string & device)
{
  bool success = false;
  // blabla hardware connection on device must return success
  success = true;
  connected_ = success;
}

void Interface::disconnect()
{
  // call hardware close link
  servoIds_.clear();
  connected_ = false;
}

void Interface::startController()
{
  // start and store shared ptr of controller instance
  // should start controller thread connected to hardware command and sensor feedback?
  // how to handle if controller cuts on its own gracefully?
}

void Interface::detectSerialPorts()
{
  // should be hardware->detect
  // for now using Qt explicitely
  for(const auto & info : QSerialPortInfo::availablePorts())
  {
    serialPorts_.push_back(info.portName().toStdString());
  }
}

void Interface::detectServos()
{
  // hardcoded test
  servoIds_ = {1, 2, 5, 7, 9};
}

ServoInfo Interface::getServoData(int id)
{
  // hardcoded test
  ServoInfo servo;

  servo.id = id;
  servo.voltage = 24.0;
  servo.current = 0.35;

  servo.kp = 10.;
  servo.ki = 0.;
  servo.kd = 2.;

  servo.position = id * 15.;
  servo.torque = true;

  return servo;
}
