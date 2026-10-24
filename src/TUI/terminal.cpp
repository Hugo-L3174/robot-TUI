#include "terminal.h"

using namespace ftxui;

TerminalApp::TerminalApp(Interface & interface) : interface_(interface), screen_(App::Fullscreen())
{
  dataPanel_ = std::make_unique<TUI::DataPanel>(interface_, screen_);
  connectionPanel_ = std::make_unique<TUI::ConnectionPanel>(interface_, screen_);
  controllerPanel_ = std::make_unique<TUI::ControllerPanel>(interface_, screen_);

  // Having everything in a container handles interaction and focus of the elements
  // Basically only putting interactive elements here, output only elements don't need focus
  mainContainer_ = Container::Vertical({connectionPanel_->Container(), controllerPanel_->Container()});

  // Then the renderer handles the actual layout
  mainRenderer_ = Renderer(mainContainer_,
                           [&]
                           {
                             return vbox({connectionPanel_->Renderer()->Render(), separator(),
                                          hbox({dataPanel_->Renderer()->Render(), separator(),
                                                controllerPanel_->Renderer()->Render() | flex})});
                           });
}

TerminalApp::~TerminalApp()
{
  screen_.Exit();
}

void TerminalApp::startRendering()
{
  screen_.Loop(mainRenderer_);
}
