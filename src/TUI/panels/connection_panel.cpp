#include "panels/connection_panel.h"
// for controllers enum
#include "connection/external_connection.h"
#include <magic_enum/magic_enum.hpp>

using namespace Panels;

ConnectionPanel::ConnectionPanel(Interface & interface, ftxui::App & screen)
: TerminalPanel(interface, screen), interfaceIndex_(0), selectedAddress_(0)
{
  buildContainer();
  buildRenderer();
}

ConnectionPanel::~ConnectionPanel() {}

void ConnectionPanel::buildContainer()
{
  constexpr auto connectionNamesArray = magic_enum::enum_names<ConnectionType>();
  std::vector<std::string> connectionNames{connectionNamesArray.begin(), connectionNamesArray.end()};

  connectionSelector_ = ftxui::Radiobox(connectionNames, &interfaceIndex_);

  // @todo for now only getting serial type of connection
  // should be generic function depending on selected interfaceIndex_
  addressSelector_ = ftxui::Dropdown(&interface_.getSerialPorts(), &selectedAddress_);

  connectButton_ = ftxui::Button("Connect",
                                 [&]
                                 {
                                   // stopServoUpdateThread();
                                   if(interfaceIndex_ == 0)
                                   {
                                     interface_.connectionType_ = ConnectionType::Serial;
                                     interface_.connect(interface_.getSerialPorts()[selectedAddress_]);
                                   }
                                   else
                                   {
                                     // @todo
                                   }
                                   // Resetting and refilling servo map
                                   // servoMap_.clear();
                                   // for(const auto & id : interface_.getServoIds())
                                   // {
                                   //   servoMap_.emplace(id, interface_.getServoData(id));
                                   // }
                                   // startServoUpdateThread();
                                   // dataPanel_->startServoUpdateThread();
                                 });

  // @todo can this be greyed out when a controller is running ?
  disconnectButton_ = ftxui::Button("Disconnect",
                                    [&]
                                    {
                                      //  stopServoUpdateThread();
                                      //  dataPanel_->stopServoUpdateThread();
                                      interface_.disconnect();
                                      //  servoMap_.clear();
                                    });

  servoOnButton_ = ftxui::Button("Servos ON",
                                 [&]
                                 {
                                   for(const auto & id : interface_.getServoIds())
                                   {
                                     interface_.setServoOn(id, true);
                                   }
                                 });
  servoOffButton_ = ftxui::Button("Servos OFF",
                                  [&]
                                  {
                                    for(const auto & id : interface_.getServoIds())
                                    {
                                      interface_.setServoOn(id, false);
                                    }
                                  });

  panelContainer_ = ftxui::Container::Horizontal(
      {connectionSelector_, addressSelector_, connectButton_, disconnectButton_, servoOnButton_, servoOffButton_});
}

void ConnectionPanel::buildRenderer()
{
  panelRenderer_ = ftxui::Renderer(
      panelContainer_,
      [&]
      {
        if(!interface_.connected())
        {
          ftxui::Element config;

          if(interfaceIndex_ == 0)
          {
            config = ftxui::hbox({
                ftxui::text("Device: "),
                addressSelector_->Render() | ftxui::flex,
                ftxui::separator(),
                connectButton_->Render() | ftxui::flex,
            });
          }
          else
          {
          }

          return ftxui::window(ftxui::text("Connection"), ftxui::hbox({
                                                              connectionSelector_->Render(),
                                                              ftxui::separator(),
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

        return ftxui::window(ftxui::text("Connection"), ftxui::hbox({ftxui::vbox({
                                                                         ftxui::text("Connected : " + status),
                                                                         ftxui::separator(),
                                                                         disconnectButton_->Render() | ftxui::flex,
                                                                     }),
                                                                     ftxui::separator(), servoOnButton_->Render(),
                                                                     ftxui::separator(), servoOffButton_->Render()}));
      });
}
