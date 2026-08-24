#pragma once
#include "controllers.h"

class ManualController : public ControllerInterface
{
public:
  ManualController();

  bool runControl();
  void startControlThread();
  void stopControlThread();

private:
};
