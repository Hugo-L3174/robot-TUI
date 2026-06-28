#include <iostream>

#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

int main()
{
  using namespace ftxui;

  // test 2
  // Define the document structure
  Element document = vbox({
      text("FTXUI Getting Started") | bold | center,
      separator(),
      hbox({
          text("Left Panel") | border,
          vbox({
              text("Main Content Area") | flex,
              separator(),
              text("Footer Information") | dim,
          }) | border
              | flex,
      }) | flex,
  });

  // Create the screen and render
  auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
  Render(screen, document);
  screen.Print();

  // test 3
  std::vector<std::string> entries = {
      "Entry 1",
      "Entry 2",
      "Entry 3",
  };
  int selected = 0;

  // Create a menu component
  auto menu = Menu(&entries, &selected);

  // You can decorate components using the pipe operator.
  auto component = menu | border;

  // Start the main loop
  auto app = App::TerminalOutput();
  app.Loop(component);

  return EXIT_SUCCESS;
}
