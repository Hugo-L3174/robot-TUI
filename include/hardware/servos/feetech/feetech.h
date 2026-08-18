#pragma once

#include <cstdint>
#include <vector>

#include "servos/servos.h"
#include "sms_sts.h"

class FeetechServos : public ServoHardware
{
public:
  FeetechServos();

  bool openLink(const std::string & device) override;
  bool closeLink() override;
  std::vector<int> detectServos() override;

  ServoInfo getServoData(int id) override;

private:
  serial_t * serial_;
  feetech_servo::SCSerial * scserial_;
  feetech_servo::SMS_STS * sms_sts_serial_;
};
