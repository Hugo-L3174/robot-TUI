#pragma once
#include "controllers.h"

/**
 * @brief The ManualController implementation provides a simple motor by motor command interface, mainly used for
 * testing.
 * In practice it is not implemented as it is bypassed by the interface to control the motors directly. The structure
 * stays so that everything is generic.
 *
 */
class ManualController : public ControllerInterface
{
public:
  ManualController();

  bool runControl(std::mutex & stateMutex,
                  const std::vector<ServoInfo> & states,
                  std::mutex & commandMutex,
                  std::vector<ServoInfo> & commands) override;
  bool startControl() override;
  bool stopControl() override;

private:
};
