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

  /////////////////////////////////////////////////////////////////////////////
  // Background update thread
  /////////////////////////////////////////////////////////////////////////////

  // std::thread updater(
  //     [&]
  //     {
  //       while(running)
  //       {
  //         if(interface.connected)
  //         {
  //           for(auto & servo : interface.servos)
  //           {
  //             servo = ReadServoInfo(servo.id);
  //           }

  //           screen.PostEvent(Event::Custom);
  //         }

  //         std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //       }
  //     });

  // running = false;
  // updater.join();

  auto interface = Interface();
  interface.detectSerialPorts();
  auto term = TerminalApp(interface);
  term.startRendering();
  return EXIT_SUCCESS;
}
