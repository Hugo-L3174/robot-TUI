#include "panels/connection_panel.h"
// for controllers enum
#include "connection/external_connection.h"
#include <magic_enum/magic_enum.hpp>

using namespace TUI;

ConnectionPanel::ConnectionPanel(Interface & interface, ftxui::App & screen)
: TerminalPanel(interface, screen), interfaceIndex_(0), selectedSerial_(0), servoType_(0)
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

  // showing the serial selector only if serial connection is selected
  serialAddressSelector_ =
      ftxui::Dropdown(&interface_.getSerialPorts(), &selectedSerial_)
      | ftxui::Maybe([&] { return static_cast<ConnectionType>(interfaceIndex_) == ConnectionType::Serial; });

  constexpr auto motorTypeArray = magic_enum::enum_names<MotorType>();
  std::vector<std::string> motorTypeNames{motorTypeArray.begin(), motorTypeArray.end()};

  servoTypeSelector_ = ftxui::Radiobox(motorTypeNames, &servoType_);

  connectButton_ = ftxui::Button("Connect",
                                 [&]
                                 {
                                   switch(static_cast<ConnectionType>(interfaceIndex_))
                                   {
                                     case ConnectionType::Serial:
                                       interface_.connectionType_ = ConnectionType::Serial;
                                       interface_.connect(static_cast<MotorType>(servoType_),
                                                          interface_.getSerialPorts()[selectedSerial_]);
                                       updatePanelControls(interface_.connected());
                                       break;

                                     default:
                                       break;
                                   }
                                 });

  // @todo can this be greyed out when a controller is running ?
  disconnectButton_ = ftxui::Button("Disconnect",
                                    [&]
                                    {
                                      interface_.disconnect();
                                      updatePanelControls(false);
                                    });

  servoOnButton_ = ftxui::Button("Servos ON",
                                 [&]
                                 {
                                   for(const auto & id : interface_.getServoIds())
                                   {
                                     interface_.setServoOn(id, true);
                                   }
                                 })
                   | ftxui::Maybe([&] { return interface_.connected(); });
  servoOffButton_ = ftxui::Button("Servos OFF",
                                  [&]
                                  {
                                    for(const auto & id : interface_.getServoIds())
                                    {
                                      interface_.setServoOn(id, false);
                                    }
                                  })
                    | ftxui::Maybe([&] { return interface_.connected(); });

  // Initialize the container empty then call the update function
  panelContainer_ = ftxui::Container::Horizontal({});
  updatePanelControls(interface_.connected());
}

void ConnectionPanel::updatePanelControls(bool connected)
{
  // Keep the interactive tree in sync with the rendered layout so hidden controls
  // cannot retain stale hit boxes or receive keyboard focus.
  panelContainer_->DetachAllChildren();
  if(connected)
  {
    panelContainer_->Add(disconnectButton_);
    panelContainer_->Add(servoOnButton_);
    panelContainer_->Add(servoOffButton_);
    panelContainer_->SetActiveChild(disconnectButton_);
  }
  else
  {
    panelContainer_->Add(connectionSelector_);
    panelContainer_->Add(serialAddressSelector_);
    panelContainer_->Add(servoTypeSelector_);
    panelContainer_->Add(connectButton_);
    panelContainer_->SetActiveChild(connectionSelector_);
  }
}

void ConnectionPanel::buildRenderer()
{
  panelRenderer_ = ftxui::Renderer(
      panelContainer_,
      [&]
      {
        if(!interface_.connected())
        {
          ftxui::Element connectionConfig;

          switch(static_cast<ConnectionType>(interfaceIndex_))
          {
            case ConnectionType::Serial:
              connectionConfig = ftxui::hbox({ftxui::vbox({
                  ftxui::text("Serial device:"),
                  serialAddressSelector_->Render() | ftxui::flex,
              })});
              break;

            default:
              connectionConfig = ftxui::hbox({ftxui::text("No available parameters.")});
              break;
          }

          return ftxui::window(ftxui::text("Connection"),
                               ftxui::vbox({ftxui::hbox({
                                                ftxui::vbox({
                                                    ftxui::text("Connection link:"),
                                                    connectionSelector_->Render(),
                                                }),
                                                ftxui::separator(),
                                                connectionConfig,
                                                ftxui::separator(),
                                                ftxui::vbox({
                                                    ftxui::text("Servo motor type:"),
                                                    servoTypeSelector_->Render(),
                                                }),
                                            }),
                                            ftxui::separator(), connectButton_->Render() | ftxui::flex}));
        }
        else // if already connected
        {
          std::string connectionParam;
          switch(static_cast<ConnectionType>(interface_.connectionType_))
          {
            case ConnectionType::Serial:
              connectionParam = interface_.getSerialPorts()[selectedSerial_];
              break;

            default:
              connectionParam = "No parameter";
              break;
          }

          return ftxui::window(
              ftxui::text("Connection"),
              ftxui::hbox(
                  {ftxui::vbox({
                       ftxui::text("Connected") | ftxui::color(ftxui::Color::Green),
                       ftxui::text("Link type: "
                                   + std::string(magic_enum::enum_name(static_cast<ConnectionType>(interfaceIndex_)))),
                       ftxui::text("Parameter: " + connectionParam),
                       ftxui::text("Servo: " + std::string(magic_enum::enum_name(static_cast<MotorType>(servoType_)))),
                       ftxui::separator(),
                       disconnectButton_->Render() | ftxui::flex,
                   }),
                   ftxui::separator(), servoOnButton_->Render(), ftxui::separator(), servoOffButton_->Render()}));
        }
      });
}
