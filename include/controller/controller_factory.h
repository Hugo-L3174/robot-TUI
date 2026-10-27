#pragma once

#include "controllers.h"
#include <memory>

enum class ControllerType
{
  Manual,
  External,
#ifdef ROBOT_TUI_HAS_MC_RTC
  mc_rtc,
#endif
};

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type,
                                                         const ControllerConfig & config,
                                                         std::shared_ptr<spdlog::logger> logger = {});
