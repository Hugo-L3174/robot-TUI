#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <mutex>
#include <thread>

#include "interface.h"

class TerminalApp
{
public:
  explicit TerminalApp(Interface & interface);

  void startRendering();

  void startServoUpdateThread();
  void stopServoUpdateThread();

private:
  Interface & interface_;

  // Main app object
  ftxui::App screen_;

  // Menus
  ftxui::Component connectionSelector_;
  ftxui::Component addressSelector_;
  // @todo add controller menu where path to a config can be passed (should be optional)

  // Buttons
  ftxui::Component connectButton_;
  ftxui::Component disconnectButton_;
  ftxui::Component searchButton_;
  // @todo add controller start button that calls interface with passed config

  // Panels
  ftxui::Component connectionPanel_;
  ftxui::Component servoPanel_;

  ftxui::Component mainContainer_;
  ftxui::Component mainRenderer_;

  // Servos data buffer for display is owned by the frontend
  std::map<int, ServoInfo> servoMap_;
  mutable std::mutex servoMapMutex_;

  std::jthread servoDataUpdater_;

  // User set params
  int interfaceIndex_;
  int selectedAddress_;

  // @todo should I make constructor functions for each component instead of building it all in main constructor ?
};
