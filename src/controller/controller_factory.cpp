#include "controller_factory.h"
#include "external.h"
#include "manual.h"

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type)
{
  switch(type)
  {
    case ControllerType::Manual:
      return std::make_unique<ManualController>();
    case ControllerType::External:
      return std::make_unique<ExternalController>();
    default:
      return nullptr;
  }
}
