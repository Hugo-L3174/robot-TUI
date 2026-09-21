#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <mutex>
#include <thread>

#include "interface.h"
// for controllers enum
#include "controller_factory.h"

class TerminalApp
{
public:
  explicit TerminalApp(Interface & interface);

  void startRendering();

  void startServoUpdateThread();
  void stopServoUpdateThread();

  // Request controller creation to the interface and populate the controllerPanelRenderer_
  void createController(ControllerType type);
  // Stops current controller and remove panel elements
  void detachController();

private:
  Interface & interface_;

  // Main app object
  ftxui::App screen_;
  ftxui::Component mainContainer_;
  ftxui::Component mainRenderer_;

  /********** Connection panel **********/
  // Container to hold the connection elements
  ftxui::Component connectionContainer_;
  ftxui::Component connectionPanelRenderer_;

  int interfaceIndex_;
  int selectedAddress_;
  ftxui::Component connectionSelector_;
  ftxui::Component addressSelector_;
  ftxui::Component connectButton_;
  ftxui::Component disconnectButton_;
  ftxui::Component searchButton_;

  /********** Servo data panel **********/
  ftxui::Component servoPanelRenderer_;

  // Servos data buffer for display is owned by the frontend
  std::map<int, ServoInfo> servoMap_;
  mutable std::mutex servoMapMutex_;
  std::jthread servoDataUpdater_;

  /********** Controller panel **********/
  // Container to hold the controller elements
  ftxui::Component controllerContainer_;
  ftxui::Component controllerPanelRenderer_;

  int selectedController_;
  ftxui::Component controllerSelector_;
  ftxui::Component startControlButton_;
  ftxui::Component stopControlButton_;

  // Manual controller slider values
  std::vector<double> sliderVals_;

  // @todo add controller menu where path to a config can be passed (should be optional)
  // @todo add controller start button that calls interface with passed config

  // @todo should I make constructor functions for each component instead of building it all in main constructor ?
};
