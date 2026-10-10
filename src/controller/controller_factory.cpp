#include "controller_factory.h"
#include "external.h"
#include "manual.h"

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type, const ControllerConfig & config)
{
  switch(type)
  {
    case ControllerType::Manual:
      return std::make_unique<ManualController>();
    case ControllerType::External:
    {
      // check if passed config was created as the correct one, otherwise convert
      // const ExternalControllerConfig * externalConfig = dynamic_cast<const ExternalControllerConfig *>(&config);
      // if (externalConfig == nullptr)
      // {

      // }
      // else
      // {
      //   /* code */
      // }
      // return nullptr if arguments are incorrect

      // @fixme for now explicitely building an external controller config to avoid downcasting issues
      ExternalControllerConfig externalConfig;
      return std::make_unique<ExternalController>(externalConfig);
    }
    default:
      return nullptr;
  }
}
