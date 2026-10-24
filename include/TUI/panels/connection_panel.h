#pragma once

#include "panels/panels.h"

namespace TUI
{
class ConnectionPanel : public TerminalPanel
{
public:
  explicit ConnectionPanel(Interface & interface, ftxui::App & screen);
  ~ConnectionPanel();

  void buildContainer() override;
  void buildRenderer() override;

private:
  int interfaceIndex_;
  int selectedAddress_;
  ftxui::Component connectionSelector_;
  ftxui::Component addressSelector_;
  ftxui::Component connectButton_;
  ftxui::Component disconnectButton_;
  ftxui::Component servoOnButton_;
  ftxui::Component servoOffButton_;
};

} // namespace TUI
