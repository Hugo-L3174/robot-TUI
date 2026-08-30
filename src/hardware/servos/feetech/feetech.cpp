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

ServoInfo FeetechServos::getServoData(const int & id)
{
  ServoInfo servoData;

  servoData.id = id;

  // feetech servos output integer value over a 4096 range (12 bit encoding)
  // --> rad pos = int pos * 2pi / 4096
  servoData.position = (sms_sts_serial_->read_position(id) * 2 * M_PI) / 4096;
  servoData.velocity = sms_sts_serial_->read_speed(id);
  servoData.torque = sms_sts_serial_->read_load(id);

  servoData.voltage = sms_sts_serial_->read_voltage_double(id);
  servoData.current = sms_sts_serial_->read_current(id) * 6.5; // convert to mA
  servoData.temperature = sms_sts_serial_->read_temperature(id);
  servoData.torqueEnabled = true;

  servoData.servoGains.kp = 10.;
  servoData.servoGains.ki = 0.;
  servoData.servoGains.kd = 2.;

  servoData.servoTargets.pos = sms_sts_serial_->read_goal(id);
  servoData.servoTargets.vel = 0.;
  servoData.servoTargets.acc = 0.;

  return servoData;
}

int FeetechServos::posTargetToServo(const double & posTarget)
{
  // target needs to be converted from double to int 0~4095
  const double unitStep = 2. * M_PI / 4096;
  // normalize target to 0-2pi range
  double normalizedTargetPos = std::fmod(posTarget, 2.0 * M_PI);
  if(normalizedTargetPos < 0.)
  {
    normalizedTargetPos += 2.0 * M_PI;
  }
  // scale and clamp to 0-4095 range
  int position = std::clamp(static_cast<int>(normalizedTargetPos * unitStep), 0, 4095);
  return position;
}

int FeetechServos::velTargetToServo(const double & velTarget)
{
  const double unitStep = 2. * M_PI / 4096;
  // target vel needs to be converted to int 0~3400 for target speed in unitStep/s
  // the value has to be unsigned, the firmware automatically signs it according to the position error
  // also clamping starting at 1 because 0 is not zero velocity but maximum velocity
  int speed = std::clamp(static_cast<int>(std::abs(velTarget * unitStep)), 1, 3400);
  return speed;
}

bool FeetechServos::setServoTarget(const int & id, const ServoInfo::targets & target)
{
  // no feedforward acceleration on sts3215, using 0 to deactivate hardware acceleration ramp
  return sms_sts_serial_->write_pos_ex(id, posTargetToServo(target.pos), velTargetToServo(target.vel));
}

bool FeetechServos::setServoTargets(const std::vector<int> & ids, std::vector<ServoInfo::targets> targets)
{
  // convert to expected uint8_t format
  std::vector<uint8_t> servoIds;
  servoIds.assign(ids.begin(), ids.end());

  std::vector<int16_t> targetsPos;
  std::vector<uint16_t> targetsVel;
  for(const auto & target : targets)
  {
    targetsPos.emplace_back(posTargetToServo(target.pos));
    targetsVel.emplace_back(velTargetToServo(target.vel));
  }
  sms_sts_serial_->sync_write_pos_ex(servoIds.data(), ids.size(), targetsPos.data(), targetsVel.data(),
                                     std::vector<uint8_t>(ids.size()).data()); // zero acc vector
}
