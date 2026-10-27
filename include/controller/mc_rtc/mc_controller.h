#pragma once
#include "controllers.h"

#include <memory>

namespace mc_control
{
struct MCGlobalController;
}

struct MCControllerConfig : ControllerConfig
{
  // No/empty config uses the default mc_rtc config files
  std::string configurationFile;
  // empty uses main robot
  std::string robotName;
};

class MCController : public ControllerInterface
{
public:
  explicit MCController(const MCControllerConfig & config, std::shared_ptr<spdlog::logger> logger = {});
  ~MCController() override;

  bool runControl(std::mutex & stateMutex,
                  const std::vector<ServoInfo> & states,
                  std::mutex & commandMutex,
                  std::vector<ServoInfo> & commands) override;
  bool startControl() override;
  bool stopControl() override;

private:
  MCControllerConfig config_;
  std::unique_ptr<mc_control::MCGlobalController> controller_;
  bool initialized_ = false;
};
