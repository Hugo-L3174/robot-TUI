#pragma once
#include "controllers.h"

class MCController : public ControllerInterface
{
public:
  MCController();

  bool runControl();
  void startControlThread();
  void stopControlThread();

private:
};

// XXX Copy structure of G1Control.h ?
