#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "interface.h"

#include "panels/connection_panel.h"
#include "panels/controller_panel.h"
#include "panels/data_panel.h"

class TerminalApp
{
public:
  explicit TerminalApp(Interface & interface);
  ~TerminalApp();

  void startRendering();

private:
  Interface & interface_;

  // Main app object
  ftxui::App screen_;
  ftxui::Component mainContainer_;
  ftxui::Component mainRenderer_;

  // data and controller panels split component
  ftxui::Component split_;

  /********** Connection panel **********/
  std::unique_ptr<TUI::ConnectionPanel> connectionPanel_;

  /********** Servo data panel **********/
  std::unique_ptr<TUI::DataPanel> dataPanel_;

  /********** Controller panel **********/
  std::unique_ptr<TUI::ControllerPanel> controllerPanel_;

  // @todo add controller menu where path to a config can be passed (should be optional)
  // @todo add controller start button that calls interface with passed config

  // @todo should I make constructor functions for each component instead of building it all in main constructor ?
};
