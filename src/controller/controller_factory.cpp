#include "controller_factory.h"
#include "external.h"
#include "manual.h"
#include "mc_rtc/mc_controller.h"

std::unique_ptr<ControllerInterface> setupControllerType(ControllerType type,
                                                         const ControllerConfig & config,
                                                         std::shared_ptr<spdlog::logger> logger)
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
      return std::make_unique<ExternalController>(externalConfig, std::move(logger));
    }
#ifdef ROBOT_TUI_HAS_MC_RTC
    case ControllerType::mc_rtc:
    {
      // if(const auto * mcConfig = dynamic_cast<const MCControllerConfig *>(&config))
      // {
      //   return std::make_unique<MCController>(*mcConfig, std::move(logger));
      // }
      MCControllerConfig MCConfig;
      return std::make_unique<MCController>(MCConfig, std::move(logger));
    }
#endif
    default:
      return nullptr;
  }
}
