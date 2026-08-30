#pragma once

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

  ServoInfo getServoData(const int & id) override;
  bool setServoTarget(const int & id, const ServoInfo::targets & target) override;
  // using synchronized write of all targets
  bool setServoTargets(const std::vector<int> & ids, std::vector<ServoInfo::targets> targets) override;

private:
  // convert incoming double position target to expected feetech int pos target
  int posTargetToServo(const double & posTarget);
  // convert incoming double velocity target to expected feetech int vel target
  int velTargetToServo(const double & velTarget);
  serial_t * serial_;
  feetech_servo::SCSerial * scserial_;
  feetech_servo::SMS_STS * sms_sts_serial_;
};
