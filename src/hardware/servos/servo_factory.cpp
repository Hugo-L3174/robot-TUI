#include "servos/servo_factory.h"
#include "servos/feetech/feetech.h"
#include "servos/simulated.h"

std::unique_ptr<ServoHardware> setupServoType(MotorType type)
{
  switch(type)
  {
    case MotorType::Simulated:
      return std::make_unique<SimulatedServos>();
    case MotorType::Feetech:
      return std::make_unique<FeetechServos>();
    default:
      return nullptr;
  }
}
