#include "mc_rtc/mc_controller.h"

#include <mc_control/mc_global_controller.h>

#include <algorithm>
#include <exception>
#include <vector>

MCController::MCController(const MCControllerConfig & config, std::shared_ptr<spdlog::logger> logger)
: ControllerInterface(config, std::move(logger)), config_(config)
{
}

MCController::~MCController() = default;

bool MCController::startControl()
{
  try
  {
    if(!controller_)
    {
      // Reroute mc_rtc's default spdlog sinks to this one so that it can be displayed in the TUI
      mc_rtc::log::details::info().sinks() = logger_->sinks();
      mc_rtc::log::details::success().sinks() = logger_->sinks();
      mc_rtc::log::details::cerr().sinks() = logger_->sinks();
      // Create global controller
      controller_ = std::make_unique<mc_control::MCGlobalController>(config_.configurationFile, nullptr);
      config_.timestep = controller_->timestep();
      if(logger_)
      {
        logger_->info("mc_rtc controller '{}' loaded with timestep {}", controller_->current_controller(),
                      config_.timestep);
      }
    }
    initialized_ = false;
    return true;
  }
  catch(const std::exception & e)
  {
    controller_.reset();
    if(logger_)
    {
      logger_->error("Failed to start mc_rtc controller: {}", e.what());
    }
    return false;
  }
}

bool MCController::stopControl()
{
  initialized_ = false;
  controller_.reset();
  return true;
}

bool MCController::runControl(std::mutex & stateMutex,
                              const std::vector<ServoInfo> & states,
                              std::mutex & commandMutex,
                              std::vector<ServoInfo> & commands)
{
  if(!controller_)
  {
    return false;
  }

  // copy read only state data
  std::vector<ServoInfo> stateSnapshot;
  {
    std::lock_guard<std::mutex> lock(stateMutex);
    stateSnapshot = states;
  }

  try
  {
    if(logger_)
    {
      logger_->info("running mc_rtc controller");
    }

    auto & mc_controller = controller_->controller();
    auto & robot =
        config_.robotName.empty() ? mc_controller.robots().robot() : mc_controller.robots().robot(config_.robotName);
    const auto & jointOrder = robot.refJointOrder();
    if(stateSnapshot.size() != jointOrder.size())
    {
      if(logger_)
      {
        logger_->error("mc_rtc robot '{}' has {} joints but TUI supplied {} states", robot.name(), jointOrder.size(),
                       stateSnapshot.size());
      }
      return false;
    }

    // Initialize vectors to pass state to mc_rtc
    std::vector<double> positions;
    std::vector<double> velocities;
    std::vector<double> torques;
    positions.reserve(stateSnapshot.size());
    velocities.reserve(stateSnapshot.size());
    torques.reserve(stateSnapshot.size());
    for(const auto & state : stateSnapshot)
    {
      positions.push_back(state.position);
      velocities.push_back(state.velocity);
      torques.push_back(state.torque);
    }

    // initialize if needed
    if(!initialized_)
    {
      controller_->init({{robot.name(), positions}});
      if(logger_)
      {
        logger_->info("called init correctly");
      }
      initialized_ = true;
    }

    // Write state
    controller_->setEncoderValues(robot.name(), positions);
    controller_->setEncoderVelocities(robot.name(), velocities);
    controller_->setJointTorques(robot.name(), torques);

    if(!controller_->run())
    {
      return false;
    }
    if(logger_)
    {
      logger_->info("ctl run succeeded !");
    }

    const auto & q = robot.mbc().q;
    const auto & alpha = robot.mbc().alpha;
    std::vector<double> targetPositions;
    std::vector<double> targetVelocities;
    targetPositions.reserve(jointOrder.size());
    targetVelocities.reserve(jointOrder.size());
    for(const auto & joint : jointOrder)
    {
      const auto jointId = robot.jointIndexByName(joint);
      targetPositions.push_back(q.at(jointId).at(0));
      targetVelocities.push_back(alpha.at(jointId).at(0));
    }

    {
      std::lock_guard<std::mutex> lock(commandMutex);
      if(commands.size() != stateSnapshot.size())
      {
        if(logger_)
        {
          logger_->error("mc_rtc command count {} does not match state count {}", commands.size(),
                         stateSnapshot.size());
        }
        return false;
      }
      for(std::size_t i = 0; i < commands.size(); ++i)
      {
        commands[i].servoTargets.pos = targetPositions[i];
        commands[i].servoTargets.vel = targetVelocities[i];
      }
    }
    return true;
  }
  catch(const std::exception & e)
  {
    if(logger_)
    {
      logger_->error("mc_rtc control step failed: {}", e.what());
    }
    return false;
  }
}
