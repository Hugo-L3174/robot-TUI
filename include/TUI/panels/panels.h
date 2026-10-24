#pragma once

#include "interface.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

namespace TUI
{

class TerminalPanel
{
public:
  TerminalPanel(Interface & interface, ftxui::App & screen) : interface_(interface), screen_(screen){};
  virtual ~TerminalPanel() = default;

  ftxui::Component & Container()
  {
    return panelContainer_;
  }
  ftxui::Component & Renderer()
  {
    return panelRenderer_;
  }
  virtual void buildContainer() = 0;
  virtual void buildRenderer() = 0;

protected:
  Interface & interface_;
  ftxui::App & screen_;
  ftxui::Component panelContainer_;
  ftxui::Component panelRenderer_;
};

} // namespace TUI
