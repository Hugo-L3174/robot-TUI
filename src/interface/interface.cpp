#include "interface.h"

// temp include for testing
#include <QSerialPortInfo>

Interface::Interface() : connected_(false)
{
  serial_ = serial_new();
  scserial_ = new feetech_servo::SCSerial(serial_);
  sms_sts_serial_ = new feetech_servo::SMS_STS(scserial_);
}

void Interface::connect(const std::string & device)
{
  // Default parameters that should be set by terminal or a loaded config file
  int baudRate = 1000000;
  connected_ = serial_open(serial_, device.c_str(), baudRate) < 0 ? false : true;
}

void Interface::disconnect()
{
  // call hardware close link
  serial_close(serial_);
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
  servoIds_.clear();
  int searchId = 0;
  // @fixme find a non blocking way for doing this
  // in theory id can go up to 255 for a single bus, in practice I know I have just 6 for now
  int ret;
  while(searchId < 7 /*256*/)
  {
    ret = scserial_->ping(searchId);
    if(0 < ret)
    {
      servoIds_.push_back(ret);
    }
    searchId++;
  }
}

ServoInfo Interface::getServoData(int id)
{
  // hardcoded test
  ServoInfo servo;

  servo.id = id;

  servo.voltage = sms_sts_serial_->read_voltage(id);
  servo.current = sms_sts_serial_->read_current(id);

  servo.kp = 10.;
  servo.ki = 0.;
  servo.kd = 2.;

  // @todo add temperature as well
  servo.temperature = sms_sts_serial_->read_temperature(id);

  servo.position = sms_sts_serial_->read_position(id);
  servo.torque = sms_sts_serial_->read_load(id);
  // @todo look if there is a torque control or position mode and if it can be read
  servo.torqueMode = true;

  return servo;
}
