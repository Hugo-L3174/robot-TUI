#pragma once

#include "controllers.h"
#include <memory>

enum class ControllerType
{
  Manual,
  MC_RTC,
};

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type);
