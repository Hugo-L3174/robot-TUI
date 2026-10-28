#include "terminal.h"

using namespace ftxui;

TerminalApp::TerminalApp(Interface & interface) : interface_(interface), screen_(App::Fullscreen())
{
  // testButton_ = Button("Test",
  //                      [&]
  //                      {
  //                        // interface_.setOffset(1, -750);
  //                        // interface_.setOffset(2, -669);
  //                        // interface_.setOffset(3, 100);
  //                        // interface_.setOffset(4, 528);
  //                        // interface_.setOffset(5, 152);
  //                        // interface_.setOffset(6, -371);
  //                      });
  dataPanel_ = std::make_unique<TUI::DataPanel>(interface_, screen_);
  connectionPanel_ = std::make_unique<TUI::ConnectionPanel>(interface_, screen_);
  controllerPanel_ = std::make_unique<TUI::ControllerPanel>(interface_, screen_);

  // @todo for now hardcoded data panel dimensions, find a way to compute requirements from the element cleanly
  // @todo can the table columns become wider if resized ?
  split_ = ResizableSplit({.main = dataPanel_->Renderer(),
                           .back = controllerPanel_->Renderer(),
                           .direction = Direction::Left,
                           .main_size = 71,
                           .min = 71,
                           .max = 100});

  // Having everything in a container handles interaction and focus of the elements
  // Basically only putting interactive elements here, output only elements don't need focus
  mainContainer_ = Container::Vertical({connectionPanel_->Container(), split_});

  // Then the renderer handles the actual layout
  mainRenderer_ =
      Renderer(mainContainer_,
               [&] {
                 return vbox({connectionPanel_->Renderer()->Render(), separator(), split_->Render() | flex});
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
