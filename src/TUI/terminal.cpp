#include "terminal.h"

#include <ftxui/dom/table.hpp>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

using namespace ftxui;

TerminalApp::TerminalApp(Interface & interface)
: interface_(interface), screen_(App::Fullscreen()), interfaceIndex_(0), selectedAddress_(0), selectedController_(0)
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

  servoOnButton_ = Button("Servos ON",
                          [&]
                          {
                            for(const auto & id : interface_.getServoIds())
                            {
                              interface_.setServoOn(id, true);
                            }
                          });
  servoOffButton_ = Button("Servos OFF",
                           [&]
                           {
                             for(const auto & id : interface_.getServoIds())
                             {
                               interface_.setServoOn(id, false);
                             }
                           });

  connectionPanelRenderer_ = Renderer(
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

        return window(text("Connection"), hbox({vbox({
                                                    text("Connected : " + status),
                                                    separator(),
                                                    disconnectButton_->Render() | flex,
                                                }),
                                                separator(), searchButton_->Render(), separator(),
                                                servoOnButton_->Render(), separator(), servoOffButton_->Render()}));
      });

  servoPanelRenderer_ = Renderer(
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
              servoData.torqueEnabled ? "ON" : "OFF"
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

        return window(text("Servos"), servoInfoTable.Render());
      });

  // Selector between available controller types
  constexpr auto controllerNamesArray = magic_enum::enum_names<ControllerType>();
  std::vector<std::string> controllerNames{controllerNamesArray.begin(), controllerNamesArray.end()};
  controllerSelector_ = Dropdown(controllerNames, &selectedController_);

  // Controller buttons
  startControlButton_ =
      ftxui::Button("Start controller", [&] { createController(static_cast<ControllerType>(selectedController_)); });
  stopControlButton_ = ftxui::Button("Stop controller", [&] { detachController(); });

  // Initialize controller panel with controller selector and start button
  controllerContainer_ = ftxui::Container::Vertical({
      controllerSelector_,
      startControlButton_,
  });
  controllerPanelRenderer_ =
      Renderer(controllerContainer_, [&] { return window(text("Controller"), controllerContainer_->Render()); });

  // Having everything in a container handles interaction and focus of the elements
  // Basically only putting interactive elements here, output only elements don't need focus
  // @todo put components in connectionContainer_, make the container child of the renderer, and put only renderers in
  // mainContainer_ to keep display and processing consistent in case of add and detach of components
  mainContainer_ =
      Container::Vertical({Container::Horizontal({connectionSelector_, addressSelector_, connectButton_,
                                                  disconnectButton_, searchButton_, servoOnButton_, servoOffButton_}),
                           controllerPanelRenderer_});

  // Then the renderer handles the actual layout
  mainRenderer_ = Renderer(
      mainContainer_,
      [&]
      {
        return vbox({connectionPanelRenderer_->Render(), separator(),
                     hbox({servoPanelRenderer_->Render(), separator(), controllerPanelRenderer_->Render() | flex})});
      });
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

void TerminalApp::createController(ControllerType type)
{
  // request controller backend creation
  interface_.setupController(type);
  // reset panel container elements
  controllerContainer_->DetachAllChildren();
  // add stop button at the top
  controllerContainer_->Add(stopControlButton_);
  // create panel depending on controller type
  switch(type)
  {
    case ControllerType::Manual:
    {
      // Manual controller adds sliders to control servo position directly
      sliderVals_.clear();
      // reserving so that the .back() values given as refs do not move
      sliderVals_.reserve(interface_.getServoIds().size());
      // separate index to keep track of place in the sliderVals_ vector
      size_t index = 0;
      std::lock_guard<std::mutex> lock(servoMapMutex_);
      for(const auto & [id, servoData] : servoMap_)
      {
        sliderVals_.emplace_back(servoData.position);
        SliderOption<double> option;
        option.value = &sliderVals_.back();
        // @fixme hardcoded 12 bit definition for the feetech servos, do it from min max params of the servo instead
        // @todo make it generic by just setting from 0 - 1 and then scale it correctly in the manual controller
        option.min = -M_PI + 2. * M_PI / 4096;
        option.max = M_PI - 2. * M_PI / 4096;
        option.increment = 2. * M_PI / 4096;
        // For the manual controller we don't need to actually implement a controller, just trigger a hardware command
        // update when the sliders are updated.
        int servoId = id;
        option.on_change = [this, servoId, index]
        {
          // @fixme why does putting a target set servo on automatically with feetech servos?
          interface_.setServoTarget(servoId, sliderVals_[index]);
        };
        controllerContainer_->Add(ftxui::Slider<double>(option));
        index++;
      }
      break;
    }
    case ControllerType::External:
      controllerContainer_->Add(ftxui::Renderer([] { return ftxui::text("External controller"); }));
      break;
  }
}

void TerminalApp::detachController()
{
  // interface_.stopController();
  // reset to controller choice
  controllerContainer_->DetachAllChildren();
  controllerContainer_->Add(controllerSelector_);
  controllerContainer_->Add(startControlButton_);
}
