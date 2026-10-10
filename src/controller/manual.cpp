#include "manual.h"

ManualController::ManualController() {}

bool ManualController::runControl(std::mutex & stateMutex,
                                  const std::vector<ServoInfo> & states,
                                  std::mutex & commandMutex,
                                  std::vector<ServoInfo> & commands)
{
  return true;
}

bool ManualController::startControl()
{
  return true;
}

bool ManualController::stopControl()
{
  return true;
}
