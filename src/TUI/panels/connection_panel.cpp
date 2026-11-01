#include "panels/connection_panel.h"
// for controllers enum
#include "connection/external_connection.h"
#include <magic_enum/magic_enum.hpp>

#include <cstdlib> // for std::getenv
#include <filesystem>
#include <set> // to check if config ids are unique
#include <stdexcept>

#include <yaml-cpp/yaml.h>

using namespace TUI;

ConnectionPanel::ConnectionPanel(Interface & interface, ftxui::App & screen)
: TerminalPanel(interface, screen), interfaceIndex_(0), selectedSerial_(0), servoType_(0), selectedConfig_(0)
{
  scanRobotConfigs();
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

  if(configFiles_.empty())
  {
    configLoader_ = ftxui::Renderer([] { return ftxui::text("No YAML configs found"); });
  }
  else
  {
    configLoader_ = ftxui::Dropdown(configFiles_, &selectedConfig_);
  }
  configLoadButton_ = ftxui::Button("Load config", [&] { loadSelectedRobotConfig(); });

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
    panelContainer_->Add(configLoader_);
    panelContainer_->Add(configLoadButton_);
    panelContainer_->Add(connectButton_);
    panelContainer_->SetActiveChild(connectionSelector_);
  }
}

void ConnectionPanel::scanRobotConfigs()
{
  // use install or src path depending on if we are installed or not
  const std::filesystem::path installPath = ROBOT_TUI_INSTALL_CONFIG_DIR;
  const std::filesystem::path srcPath = ROBOT_TUI_SRC_CONFIG_DIR;
  configDirectory_ = std::filesystem::is_directory(installPath) ? installPath.string() : srcPath.string();

  try
  {
    // Add all valid yaml files found in the decided folder
    for(const auto & entry : std::filesystem::directory_iterator(configDirectory_))
    {
      const auto path = entry.path();
      const auto extension = path.extension();

      if(entry.is_regular_file() && (extension == ".yaml"))
      {
        configFiles_.push_back(path.filename().string());
      }
    }
  }
  catch(const std::filesystem::filesystem_error & error)
  {
    configStatus_ = "Error getting config directory: " + std::string(error.what());
  }
  if(configFiles_.empty())
  {
    configStatus_ = "No config file found in " + configDirectory_;
  }
}

void ConnectionPanel::loadSelectedRobotConfig()
{
  if(configFiles_.empty())
  {
    configStatus_ = "Select a robot config first";
    return;
  }

  try
  {
    const auto path = std::filesystem::path(configDirectory_) / configFiles_[selectedConfig_];
    const YAML::Node root = YAML::LoadFile(path.string());
    const YAML::Node servos = root["servos"];
    if(!root.IsMap() || !root["name"] || !servos || !servos.IsMap() || servos.size() == 0)
    {
      // throwing to be caught in catch block with message
      throw std::runtime_error("expected a name and a non-empty servos map");
    }

    RobotConfig config{};
    config.name = root["name"].as<std::string>();
    config.actuatedJointsNb = 0;
    // using a set to check for id uniqueness
    std::set<int> servoIds;

    for(const auto & servoEntry : servos)
    {
      const std::string jointName = servoEntry.first.as<std::string>();
      const YAML::Node servo = servoEntry.second;
      const YAML::Node limits = servo["limits"];
      if(!servo["servo_id"] || !servo["q_init"] || !limits["q"]["upper"] || !limits["q"]["lower"] || !limits["torque"])
      {
        throw std::runtime_error("incomplete servo entry: " + jointName);
      }

      ServoConfig actuator{};
      actuator.jointName = jointName;
      actuator.servoId = servo["servo_id"].as<int>();
      actuator.qInit = servo["q_init"].as<double>();
      actuator.jointUpperLimit = limits["q"]["upper"].as<double>();
      actuator.jointLowerLimit = limits["q"]["lower"].as<double>();
      actuator.effortLimit = limits["torque"].as<double>();

      if(actuator.servoId <= 0 || !servoIds.insert(actuator.servoId).second)
      {
        throw std::runtime_error("servo IDs must be positive and unique (" + jointName + ")");
      }
      if(actuator.jointLowerLimit > actuator.jointUpperLimit || actuator.effortLimit < 0.)
      {
        throw std::runtime_error("invalid limits for servo: " + jointName);
      }

      config.actuators.emplace(jointName, std::move(actuator));
      config.actuatedJointsNb++;
    }

    // Load it to the interface
    interface_.setRobotConfig(config);
    configStatus_ = "Loaded robot config:\n" + interface_.robotConfig()->name;
  }
  catch(const std::exception & error)
  {
    configStatus_ = "Config load failed:\n" + std::string(error.what());
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
                  serialAddressSelector_->Render(),
              })});
              break;

            default:
              connectionConfig = ftxui::hbox({ftxui::text("No available parameters.")});
              break;
          }

          return ftxui::window(
              ftxui::text("Connection"),
              ftxui::vbox({ftxui::hbox({ftxui::vbox({
                                            ftxui::text("Connection link:"),
                                            connectionSelector_->Render(),
                                        }),
                                        ftxui::separator(), connectionConfig, ftxui::separator(),
                                        ftxui::vbox({
                                            ftxui::text("Servo motor type:"),
                                            servoTypeSelector_->Render(),
                                        }),
                                        ftxui::separator(),
                                        ftxui::hbox({ftxui::vbox({
                                                         ftxui::text("Robot config:"),
                                                         configLoader_->Render(),
                                                         configLoadButton_->Render(),
                                                     }),
                                                     ftxui::vbox({ftxui::paragraphAlignLeft(configStatus_)})})}),
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
                       ftxui::text("Robot: "
                                   + (interface_.robotConfig() ? interface_.robotConfig()->name : "Not loaded")),
                       ftxui::text("Servo: " + std::string(magic_enum::enum_name(static_cast<MotorType>(servoType_)))),
                       ftxui::separator(),
                       disconnectButton_->Render() | ftxui::flex,
                   }),
                   ftxui::separator(), servoOnButton_->Render(), ftxui::separator(), servoOffButton_->Render()}));
        }
      });
}
