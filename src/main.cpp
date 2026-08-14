#include "terminal.h"

////////////////////////////////////////////////////////////////////////////////
// RobotControllerImpl controller;

// MainWindow window({
//     .start = [&] { controller.Start(); },
//     .stop  = [&] { controller.Stop(); },
//     .change_speed = [&](int s) {
//         controller.SetSpeed(s);
//     }
// });

int main()
{

  auto interface = Interface();
  interface.detectSerialPorts();
  auto term = TerminalApp(interface);
  term.startRendering();
  return EXIT_SUCCESS;
}
