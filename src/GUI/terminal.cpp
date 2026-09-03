#include "terminal.h"

#include <ftxui/dom/table.hpp>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

using namespace ftxui;

TerminalApp::TerminalApp(Interface & interface)
: interface_(interface), screen_(App::Fullscreen()), interfaceIndex_(0), selectedAddress_(0)
{
  constexpr auto connectionNamesArray = magic_enum::enum_names<ConnectionType>();
  std::vector<std::string> connectionNames{connectionNamesArray.begin(), connectionNamesArray.end()};

  connectionSelector_ = Radiobox(connectionNames, &interfaceIndex_);

  // @todo for now only getting serial type of connection
  // should be generic function depending on selected interfaceIndex_
  addressSelector_ = Dropdown(&interface_.getSerialPorts(), &selectedAddress_);

  connectButton_ = Button("Connect",
                          [&]
                          {
                            if(interfaceIndex_ == 0)
                            {
                              interface_.connectionType_ = ConnectionType::Serial;
                              interface_.connect(interface_.getSerialPorts()[selectedAddress_]);
                            }
                            else
                            {
                              // @todo
                            }
                          });

  // @todo can this be greyed out when a controller is running ?
  disconnectButton_ = Button("Disconnect",
                             [&]
                             {
                               stopServoUpdateThread();
                               interface_.disconnect();
                               servoMap_.clear();
                             });

  // @todo should be greyed out or removed during control
  searchButton_ = Button("Search",
                         [&]
                         {
                           stopServoUpdateThread();
                           interface_.detectServos();
                           // Resetting and refilling servo map
                           servoMap_.clear();
                           for(const auto & id : interface_.getServoIds())
                           {
                             servoMap_.emplace(id, interface_.getServoData(id));
                           }
                           startServoUpdateThread();
                         });

  connectionPanel_ = Renderer(
      [&]
      {
        if(!interface_.connected())
        {
          Element config;

          if(interfaceIndex_ == 0)
          {
            config = hbox({
                text("Device: "),
                addressSelector_->Render() | flex,
                separator(),
                connectButton_->Render() | flex,
            });
          }
          else
          {
          }

          return window(text("Connection"), hbox({
                                                connectionSelector_->Render(),
                                                separator(),
                                                config,
                                            }));
        }

        std::string status;

        if(interface_.connectionType_ == ConnectionType::Serial)
        {
          status = interface_.getSerialPorts()[selectedAddress_];
        }
        else
        {
        }

        return window(text("Connection"), hbox({
                                              vbox({
                                                  text("Connected : " + status),
                                                  separator(),
                                                  disconnectButton_->Render() | flex,
                                              }),
                                              separator(),
                                              searchButton_->Render() | flex,
                                          }));
      });

  servoPanel_ = Renderer(
      [&]
      {
        if(!interface_.connected())
        {
          return window(text("Servos"), text("Open a connection first."));
        }

        std::vector<std::vector<std::string>> tableRows;
        // First row (categories)
        tableRows.push_back({"ID", "Voltage", "Current", "KP", "KD", "Position (rad)", "Position target",
                             "Velocity (rad/s)", "Torque"});

        std::unique_lock<std::mutex> lock(servoMapMutex_);
        for(const auto & [id, servoData] : servoMap_)
        {
          // clang-format off
          tableRows.push_back(
						{
							std::to_string(id),
							fmt::format("{:.4f}", servoData.voltage),
              fmt::format("{:.4f}", servoData.current),
							fmt::format("{:.2f}", servoData.servoGains.kp),
              fmt::format("{:.2f}", servoData.servoGains.kd),
							fmt::format("{:.5f}", servoData.position),
              fmt::format("{:.5f}", servoData.servoTargets.pos),
              fmt::format("{:.5f}", servoData.velocity),
              servoData.torque ? "ON" : "OFF"
						}
					);
          // clang-format on
        }
        lock.unlock();

        auto servoInfoTable = Table(tableRows);

        // Make first row bold with a double border.
        servoInfoTable.SelectRow(0).Decorate(bold);
        servoInfoTable.SelectRow(0).Decorate(hcenter);
        servoInfoTable.SelectRow(0).SeparatorVertical(LIGHT);
        servoInfoTable.SelectRow(0).Border(DOUBLE);

        return window(text("Servos"), servoInfoTable.Render() | flex);
      });

  mainContainer_ = Container::Vertical({
      Container::Horizontal({
          connectionSelector_,
          addressSelector_,
          connectButton_,
          disconnectButton_,
          searchButton_,
      }),
  });

  mainRenderer_ = Renderer(mainContainer_,
                           [&]
                           {
                             return vbox({
                                 connectionPanel_->Render(),
                                 separator(),
                                 servoPanel_->Render(),
                             });
                           });

  // mainRenderer_ = Renderer([&] {
  //   return vbox({
  //       hbox({
  //           window(vbox({
  //               connectionPanel_->Render(),
  //           }), hflex),
  //           separatorVertical(),
  //           window(vbox({
  //               servoPanel_->Render(),
  //           }), hflex),
  //       }),
  //   });
  // });
}

void TerminalApp::startRendering()
{
  screen_.Loop(mainRenderer_);
}

void TerminalApp::startServoUpdateThread()
{
  if(servoDataUpdater_.joinable())
  {
    return;
  }

  servoDataUpdater_ = std::jthread(
      [&](std::stop_token stopToken)
      {
        while(!stopToken.stop_requested())
        {
          std::unique_lock<std::mutex> lock(servoMapMutex_);
          for(auto & [id, servoData] : servoMap_)
          {
            servoData = interface_.getServoData(id);
          }
          lock.unlock();

          // Trigger ftxui re rendering
          screen_.PostEvent(Event::Custom);

          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
}

void TerminalApp::stopServoUpdateThread()
{
  if(!servoDataUpdater_.joinable())
  {
    return;
  }
  // Set the jthread stop token
  servoDataUpdater_.request_stop();
  servoDataUpdater_.join();
}
