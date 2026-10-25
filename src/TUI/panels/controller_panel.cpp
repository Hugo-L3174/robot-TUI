#include "panels/controller_panel.h"

// for controllers enum
#include "connection/external_connection.h"
#include <magic_enum/magic_enum.hpp>

// for M_PI
#include <math.h>

using namespace TUI;

ControllerPanel::ControllerPanel(Interface & interface, ftxui::App & screen)
: TerminalPanel(interface, screen), selectedController_(0), selectedExternalConnection_(0)
{
  buildContainer();
  buildRenderer();
}

ControllerPanel::~ControllerPanel()
{
  detachController();
}

void ControllerPanel::buildContainer()
{
  // Selector between available controller types
  constexpr auto controllerNamesArray = magic_enum::enum_names<ControllerType>();
  std::vector<std::string> controllerNames{controllerNamesArray.begin(), controllerNamesArray.end()};
  controllerSelector_ = ftxui::Dropdown(controllerNames, &selectedController_);

  // Subtype selection for external controller
  constexpr auto externalConnectionsArray = magic_enum::enum_names<ExternalConnectionType>();
  std::vector<std::string> externalConnections = {externalConnectionsArray.begin(), externalConnectionsArray.end()};
  externalConnectionSelector_ =
      ftxui::Radiobox(externalConnections, &selectedExternalConnection_) | ftxui::border
      | ftxui::Maybe([&] { return static_cast<ControllerType>(selectedController_) == ControllerType::External; });

  // Controller buttons
  startControlButton_ =
      ftxui::Button("Start controller", [&] { createController(static_cast<ControllerType>(selectedController_)); });
  stopControlButton_ = ftxui::Button("Stop controller", [&] { detachController(); });

  // Logger (not added to container yet, depends on controller type)
  controllerLog_ = std::make_shared<Logger>(1000, [this] { screen_.PostEvent(ftxui::Event::Custom); });
  // Create sink to logger for this panel, to be passed to controller elements
  controllerSink_ = makeLoggerSink(controllerLog_);

  // Initialize controller panel with controller selector and start button
  panelContainer_ = ftxui::Container::Vertical({
      controllerSelector_,
      externalConnectionSelector_,
      startControlButton_,
  });
}

void ControllerPanel::buildRenderer()
{
  panelRenderer_ =
      ftxui::Renderer(panelContainer_,
                      [&]
                      {
                        if(!interface_.connected())
                        {
                          return ftxui::window(ftxui::text("Controller"), ftxui::text("Open a connection first."));
                        }
                        return ftxui::window(ftxui::text("Controller"), panelContainer_->Render());
                      });
}

void ControllerPanel::createController(ControllerType type)
{
  // reset panel container elements
  panelContainer_->DetachAllChildren();
  // add stop button at the top
  panelContainer_->Add(stopControlButton_);
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
      // std::lock_guard<std::mutex> lock(servoMapMutex_);
      for(const auto & servoData : interface_.getServoData())
      {
        sliderVals_.emplace_back(servoData.position);
        ftxui::SliderOption<double> option;
        // @fixme find a way to pass servo name as slider label
        option.value = &sliderVals_.back();
        // @fixme hardcoded 12 bit definition for the feetech servos, do it from min max params of the servo instead
        // @todo make it generic by just setting from 0 - 1 and then scale it correctly in the manual controller
        option.min = -M_PI + 2. * M_PI / 4096;
        option.max = M_PI - 2. * M_PI / 4096;
        option.increment = 2. * M_PI / 4096;
        // For the manual controller we don't need to actually implement a controller, just trigger a hardware command
        // update when the sliders are updated.
        int servoId = servoData.id;
        option.on_change = [this, servoId, index]
        {
          // @fixme why does putting a target set servo on automatically with feetech servos?
          interface_.setServoTarget(servoId, sliderVals_[index]);
        };
        panelContainer_->Add(ftxui::Slider<double>(option));
        index++;
      }
      break;
    }
    case ControllerType::External:
      // @todo make a window and display controller type on top
      panelContainer_->Add(controllerLog_);
      break;
    case ControllerType::mc_rtc:
      panelContainer_->Add(controllerLog_);
      break;
  }

  // pass logger sink to controller layer
  interface_.setupController(type, ControllerConfig{}, controllerSink_);
  // Start controller
  interface_.startController();
}

void ControllerPanel::detachController()
{
  interface_.stopController();
  // reset to controller choice
  panelContainer_->DetachAllChildren();
  panelContainer_->Add(controllerSelector_);
  panelContainer_->Add(startControlButton_);
}
