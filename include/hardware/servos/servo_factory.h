#pragma once

#include "servos.h"
#include <memory>

enum class MotorType
{
  Feetech,
};

std::unique_ptr<ServoHardware> setupServoType(MotorType type);
