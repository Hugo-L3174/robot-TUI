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
                               interface_.disconnect();
                               servoMap_.clear();
                               // @todo should stop servo polling thread
                             });

  // @todo should be greyed out or removed during control
  searchButton_ = Button("Search",
                         [&]
                         {
                           // @todo should stop any existing thread
                           interface_.detectServos();
                           // Resetting and refilling servo map
                           servoMap_.clear();
                           for(const auto & id : interface_.getServoIds())
                           {
                             servoMap_.emplace(id, interface_.getServoData(id));
                           }
                           // @todo should trigger start of polling thread
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
        tableRows.push_back({"ID", "Voltage", "Current", "KP", "KI", "Position", "Torque"});

        for(const auto & [id, servoData] : servoMap_)
        {
          // clang-format off
          tableRows.push_back(
						{
							std::to_string(id),
							fmt::format("{:.5f}", servoData.voltage),
              fmt::format("{:.5f}", servoData.current),
							fmt::format("{:.3f}", servoData.kp),
              fmt::format("{:.3f}", servoData.ki),
							fmt::format("{:.5f}", servoData.position),
              servoData.torque ? "ON" : "OFF"
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
}

void TerminalApp::startRendering()
{
  screen_.Loop(mainRenderer_);
}
