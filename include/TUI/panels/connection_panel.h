#pragma once

#include "panels/panels.h"

#include <string>
#include <vector>

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
  // Scans all yaml config files existing in the build or install config folder to fill configFiles_
  void scanRobotConfigs();
  // Parse selected yaml config and set it to interface
  void loadSelectedRobotConfig();
  // Variable to switch between connection types
  int interfaceIndex_;
  // Variable to switch on serial device
  int selectedSerial_;
  // Variable to switch on servo type
  int servoType_;
  // Variable to switch on found robot configs
  int selectedConfig_;
  // Vector of all found robot config paths
  std::vector<std::string> configFiles_;
  // Config folder (set by cmake variable)
  std::string configDirectory_;
  // Status message regarding robot config
  std::string configStatus_;
  ftxui::Component connectionSelector_;
  ftxui::Component serialAddressSelector_;
  ftxui::Component servoTypeSelector_;
  ftxui::Component connectButton_;
  ftxui::Component disconnectButton_;
  ftxui::Component servoOnButton_;
  ftxui::Component servoOffButton_;
  ftxui::Component configLoader_;
  ftxui::Component configLoadButton_;
};

} // namespace TUI
