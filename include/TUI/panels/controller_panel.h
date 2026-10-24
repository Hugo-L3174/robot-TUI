#pragma once

#include "panels/panels.h"

#include "subprocess/ProcessBuilder.hpp"
#include "subprocess/basic_types.hpp"

namespace TUI
{
class ControllerPanel : public TerminalPanel
{
public:
  explicit ControllerPanel(Interface & interface, ftxui::App & screen);
  ~ControllerPanel();

  // Build all ftxui components and variables, and return organized container
  void buildContainer() override;
  // Return renderer that uses the container
  void buildRenderer() override;

  // Request controller creation to the interface and populate the panel renderer
  void createController(ControllerType type);
  // Stops current controller and remove panel elements
  void detachController();

private:
  int selectedController_;
  int selectedExternalConnection_;
  ftxui::Component controllerSelector_;
  ftxui::Component externalConnectionSelector_;
  ftxui::Component startControlButton_;
  ftxui::Component stopControlButton_;

  // Manual controller slider values
  std::vector<double> sliderVals_;
};

} // namespace TUI
