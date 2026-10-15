#pragma once

#include "panels/panels.h"

#include "servos/servos.h"

namespace Panels
{
class DataPanel : public TerminalPanel
{
public:
  explicit DataPanel(Interface & interface, ftxui::App & screen);
  void buildContainer() override;
  void buildRenderer() override;

  void startServoUpdateThread();
  void stopServoUpdateThread();

private:
  // Servos data buffer for display
  std::vector<ServoInfo> servoData_;
  mutable std::mutex servosMutex_;
  std::jthread servoDataUpdater_;
};

} // namespace Panels
