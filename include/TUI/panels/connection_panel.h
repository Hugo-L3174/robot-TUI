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
  // Util function to update components in the container according to connection state
  void updatePanelControls(bool connected);
  // Variable to switch between connection types
  int interfaceIndex_;
  // Variable to switch on serial device
  int selectedSerial_;
  // Variable to switch on servo type
  int servoType_;
  ftxui::Component connectionSelector_;
  ftxui::Component serialAddressSelector_;
  ftxui::Component servoTypeSelector_;
  ftxui::Component connectButton_;
  ftxui::Component disconnectButton_;
  ftxui::Component servoOnButton_;
  ftxui::Component servoOffButton_;
};

} // namespace TUI
