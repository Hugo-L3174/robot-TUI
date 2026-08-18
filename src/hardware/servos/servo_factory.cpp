#include "servos/servo_factory.h"
#include "servos/feetech/feetech.h"

std::unique_ptr<ServoHardware> setupServoType(MotorType type)
{
  switch(type)
  {
    case MotorType::Feetech:
      return std::make_unique<FeetechServos>();
    default:
      return nullptr;
  }
}
