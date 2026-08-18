#include "servos/feetech/feetech.h"

// for M_PI
#include <math.h>

FeetechServos::FeetechServos()
{
  serial_ = serial_new();
  scserial_ = new feetech_servo::SCSerial(serial_);
  sms_sts_serial_ = new feetech_servo::SMS_STS(scserial_);
}

bool FeetechServos::openLink(const std::string & device)
{
  // Default parameters that should be set by terminal or a loaded config file
  int baudRate = 1000000;
  // @todo add a check not open yet
  return serial_open(serial_, device.c_str(), baudRate) < 0 ? false : true;
}

bool FeetechServos::closeLink()
{
  // @todo add a check not closed yet
  return serial_close(serial_) < 0 ? false : true;
}

std::vector<int> FeetechServos::detectServos()
{
  int searchId = 0;
  std::vector<int> servoIds;
  // @fixme find a non blocking way for doing this
  // in theory id can go up to 255 for a single bus, in practice I know I have just 6 for now
  int ret;
  while(searchId < 7 /*256*/)
  {
    ret = scserial_->ping(searchId);
    if(0 < ret)
    {
      servoIds.push_back(ret);
    }
    searchId++;
  }
  return servoIds;
}

ServoInfo FeetechServos::getServoData(int id)
{
  ServoInfo servoData;

  servoData.id = id;

  servoData.voltage = sms_sts_serial_->read_voltage(id);
  servoData.current = sms_sts_serial_->read_current(id);

  servoData.kp = 10.;
  servoData.ki = 0.;
  servoData.kd = 2.;

  servoData.temperature = sms_sts_serial_->read_temperature(id);

  // feetech servos output integer value between 0 and 4095 (12 bit encoding)
  // --> rad pos = int pos * 2pi / 4096
  servoData.position = (sms_sts_serial_->read_position(id) * 2 * M_PI) / 4096;
  servoData.torque = sms_sts_serial_->read_load(id);
  // @todo look if there is a torque control or position mode and if it can be read
  servoData.torqueMode = true;

  return servoData;
}
