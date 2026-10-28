#include "panels/data_panel.h"

#include <fmt/format.h>
#include <ftxui/dom/table.hpp>

using namespace TUI;

DataPanel::DataPanel(Interface & interface, ftxui::App & screen) : TerminalPanel(interface, screen)
{
  buildRenderer();
  startServoUpdateThread();
}

DataPanel::~DataPanel()
{
  stopServoUpdateThread();
}

void DataPanel::buildContainer()
{
  panelContainer_ = ftxui::Container::Horizontal({});
}

void DataPanel::buildRenderer()
{
  panelRenderer_ = ftxui::Renderer(
      [&]
      {
        if(!interface_.connected())
        {
          return ftxui::window(ftxui::text("Servos"), ftxui::text("Open a connection first."));
        }

        std::vector<std::vector<std::string>> tableRows;
        // First row (categories)
        tableRows.push_back({"ID", "Voltage", "Current", "KP", "KD", "Pos(rad)", "Pos target", "Vel(rad/s)", "Torque"});

        std::unique_lock<std::mutex> lock(servosMutex_);
        for(const auto & servoData : servoData_)
        {
          // clang-format off
          tableRows.push_back(
						{
							std::to_string(servoData.id),
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

        auto servoInfoTable = ftxui::Table(tableRows);

        // Make first row bold with a double border.
        servoInfoTable.SelectRow(0).Decorate(ftxui::bold);
        servoInfoTable.SelectRow(0).Decorate(ftxui::hcenter);
        servoInfoTable.SelectRow(0).SeparatorVertical(ftxui::LIGHT);
        servoInfoTable.SelectRow(0).Border(ftxui::DOUBLE);

        return window(ftxui::text("Servos"), servoInfoTable.Render());
      });
}

void DataPanel::startServoUpdateThread()
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
          std::unique_lock<std::mutex> lock(servosMutex_);
          servoData_ = interface_.getServoData();
          lock.unlock();

          // Trigger ftxui re rendering
          screen_.PostEvent(ftxui::Event::Custom);

          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
}

void DataPanel::stopServoUpdateThread()
{
  if(!servoDataUpdater_.joinable())
  {
    return;
  }
  // Set the jthread stop token
  servoDataUpdater_.request_stop();
  servoDataUpdater_.join();
}
