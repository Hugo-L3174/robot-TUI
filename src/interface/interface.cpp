#include "interface.h"

// temp include for testing
#include <QSerialPortInfo>

Interface::Interface() : connected_(false)
{
  // @todo how to handle servo hardware factory ? probably need to pass arguments from front end?
  // for now hardcoded feetech type
  setupServos(MotorType::Feetech);
}

void Interface::connect(const std::string & device)
{
  connected_ = servoHardware_->openLink(device);
}

void Interface::disconnect()
{
  servoHardware_->closeLink();
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
  // @fixme should be hardware->detect
  // for now using Qt explicitely
  for(const auto & info : QSerialPortInfo::availablePorts())
  {
    // @fixme explicitely adding /dev/ to path because qserial only returns port name
    serialPorts_.push_back("/dev/" + info.portName().toStdString());
  }
}

void Interface::detectServos()
{
  servoIds_ = servoHardware_->detectServos();
}

ServoInfo Interface::getServoData(int id)
{
  return servoHardware_->getServoData(id);
}

void Interface::setupServos(MotorType type)
{
  servoHardware_ = setupServoType(type);
  // @fixme alert on nullptr
}
