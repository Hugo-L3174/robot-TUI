#pragma once
#include "connection/external_connection.h"
#include "controllers.h"

/**
 * @brief The ExternalController implementation assumes that a controller is running somewhere else and sending the
 * resulting commands. This class implements a server to receive them and transmit to the robot.
 *
 */
class ExternalController : public ControllerInterface
{
public:
  ExternalController();

  bool runControl() override;
  void startControlThread() override;
  void stopControlThread() override;

private:
};
