#pragma once

#include "controllers.h"
#include <memory>

enum class ControllerType
{
  Manual,
  mc_rtc,
  External
};

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type,
                                                         const ControllerConfig & config,
                                                         std::shared_ptr<spdlog::logger> logger = {});
