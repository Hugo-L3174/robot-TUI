#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>

#include <magic_enum/magic_enum.hpp>

#include "interface.h"

class TerminalApp
{
public:
  explicit TerminalApp(Interface & interface);

private:
  Interface interface_;

  // Menus
  ftxui::Component connectionSelector_;
  ftxui::Component addressSelector_;

  // Buttons
  ftxui::Component connectButton_;
  ftxui::Component disconnectButton_;
  ftxui::Component searchButton_;

  // User set params
  int interfaceIndex_;
  int selectedAddress_;
};
