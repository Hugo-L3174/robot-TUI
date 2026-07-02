#include <atomic>
#include <chrono>
#include <fmt/format.h>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>

using namespace ftxui;

////////////////////////////////////////////////////////////////////////////////
// Hardware interface (to implement)
////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> DetectSerialPorts()
{
  return {
      "/dev/ttyUSB0",
      "/dev/ttyUSB1",
  };
}

bool OpenUDP(int port)
{
  (void)port;
  return true;
}

bool OpenSerial(const std::string & device)
{
  (void)device;
  return true;
}

void CloseConnection() {}

std::vector<int> SearchServos()
{
  return {1, 2, 5, 7};
}

struct ServoInfo
{
  int id = 0;

  double voltage = 0.;
  double current = 0.;

  double kp = 0.;
  double ki = 0.;
  double kd = 0.;

  double position = 0.;
  bool torque = false;
};

ServoInfo ReadServoInfo(int id)
{
  ServoInfo servo;

  servo.id = id;
  servo.voltage = 24.0;
  servo.current = 0.35;

  servo.kp = 10.;
  servo.ki = 0.;
  servo.kd = 2.;

  servo.position = id * 15.;
  servo.torque = true;

  return servo;
}

////////////////////////////////////////////////////////////////////////////////

enum class ConnectionType
{
  UDP,
  Serial,
};

struct Interface
{
  bool connected = false;

  ConnectionType connection_type = ConnectionType::UDP;

  // UDP
  std::string udp_port = "14550";

  // Serial
  std::vector<std::string> serial_ports = DetectSerialPorts();
  int serial_selected = 0;

  // Servo list
  std::vector<ServoInfo> servos;
};

////////////////////////////////////////////////////////////////////////////////

int main()
{
  auto screen = App::Fullscreen();

  Interface interface;

  std::atomic<bool> running = true;

  /////////////////////////////////////////////////////////////////////////////
  // Widgets
  /////////////////////////////////////////////////////////////////////////////

  int interface_index = 0;

  std::vector<std::string> interface_names = {
      "UDP",
      "Serial",
  };

  auto interface_selector = Radiobox(&interface_names, &interface_index);

  auto udp_port = Input(&interface.udp_port, "Port");

  auto serial_dropdown = Dropdown(&interface.serial_ports, &interface.serial_selected);

  auto connect_button = Button("Connect",
                               [&]
                               {
                                 bool success = false;

                                 if(interface_index == 0)
                                 {
                                   interface.connection_type = ConnectionType::UDP;
                                   success = OpenUDP(std::stoi(interface.udp_port));
                                 }
                                 else
                                 {
                                   interface.connection_type = ConnectionType::Serial;
                                   success = OpenSerial(interface.serial_ports[interface.serial_selected]);
                                 }

                                 interface.connected = success;
                               });

  auto disconnect_button = Button("Disconnect",
                                  [&]
                                  {
                                    CloseConnection();

                                    interface.connected = false;
                                    interface.servos.clear();
                                  });

  auto search_button = Button("Search",
                              [&]
                              {
                                interface.servos.clear();

                                auto ids = SearchServos();

                                for(const auto & id : ids)
                                {
                                  interface.servos.push_back(ReadServoInfo(id));
                                }
                              });

  /////////////////////////////////////////////////////////////////////////////
  // Background update thread
  /////////////////////////////////////////////////////////////////////////////

  std::thread updater(
      [&]
      {
        while(running)
        {
          if(interface.connected)
          {
            for(auto & servo : interface.servos)
            {
              servo = ReadServoInfo(servo.id);
            }

            screen.PostEvent(Event::Custom);
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });

  /////////////////////////////////////////////////////////////////////////////
  // Connection panel
  /////////////////////////////////////////////////////////////////////////////

  auto connection_panel = Renderer(
      [&]
      {
        if(!interface.connected)
        {
          Element config;

          if(interface_index == 0)
          {
            config = hbox({
                text("Port: "),
                udp_port->Render() | flex,
                separator(),
                connect_button->Render() | flex,
            });
          }
          else
          {
            config = hbox({
                text("Device: "),
                serial_dropdown->Render() | flex,
                separator(),
                connect_button->Render() | flex,
            });
          }

          return window(text("Connection"), hbox({
                                                interface_selector->Render(),
                                                separator(),
                                                config,
                                            }));
        }

        std::string status;

        if(interface.connection_type == ConnectionType::UDP)
        {
          status = "UDP via port : " + interface.udp_port;
        }
        else
        {
          status = interface.serial_ports[interface.serial_selected];
        }

        return window(text("Connection"), hbox({
                                              vbox({
                                                  text("Connected : " + status),
                                                  separator(),
                                                  disconnect_button->Render() | flex,
                                              }),
                                              separator(),
                                              search_button->Render() | flex,
                                          }));
      });

  /////////////////////////////////////////////////////////////////////////////
  // Servo panel
  /////////////////////////////////////////////////////////////////////////////

  auto servo_panel = Renderer(
      [&]
      {
        if(!interface.connected)
        {
          return window(text("Servos"), text("Open a connection first."));
        }

        std::vector<std::vector<std::string>> tableRows;
        // First row (categories)
        tableRows.push_back({"ID", "Voltage", "Current", "KP", "KI", "Position", "Torque"});

        for(const auto & servo : interface.servos)
        {
          // clang-format off
          tableRows.push_back(
						{
							std::to_string(servo.id),
							fmt::format("{:.5f}", servo.voltage),
              fmt::format("{:.5f}", servo.current),
							fmt::format("{:.3f}", servo.kp),
              fmt::format("{:.3f}", servo.ki),
							fmt::format("{:.5f}", servo.position),
              servo.torque ? "ON" : "OFF"
						}
					);
          // clang-format on
        }

        auto servoInfoTable = Table(tableRows);

        // Make first row bold with a double border.
        servoInfoTable.SelectRow(0).Decorate(bold);
        servoInfoTable.SelectRow(0).Decorate(hcenter);
        servoInfoTable.SelectRow(0).SeparatorVertical(LIGHT);
        servoInfoTable.SelectRow(0).Border(DOUBLE);

        return window(text("Servos"), servoInfoTable.Render() | flex);
      });

  /////////////////////////////////////////////////////////////////////////////
  // Main container
  /////////////////////////////////////////////////////////////////////////////

  auto container = Container::Vertical({
      Container::Horizontal({
          interface_selector,
          udp_port,
          serial_dropdown,
          connect_button,
          disconnect_button,
          search_button,
      }),
  });

  auto renderer = Renderer(container,
                           [&]
                           {
                             return vbox({
                                 connection_panel->Render(),
                                 separator(),
                                 servo_panel->Render(),
                             });
                           });

  screen.Loop(renderer);

  running = false;
  updater.join();

  return EXIT_SUCCESS;
}

/* @todo
Since this is supposed to be running on the robot machine (now just same pc because no embedded computer)
this should run the udp server ig? to emulate same principle
Maybe look at G1Control for a more recent example

Check why arrow navigating seems to iterate over invisible elements?
*/
