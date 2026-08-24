#include "controller_factory.h"
#include "manual.h"
#include "mc_rtc/mc_controller.h"

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type)
{
  switch(type)
  {
    case ControllerType::Manual:
      return std::make_unique<ManualController>();
    case ControllerType::MC_RTC:
      return std::make_unique<MCController>();
    default:
      return nullptr;
  }
}
