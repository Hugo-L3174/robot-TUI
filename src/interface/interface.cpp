#include "interface.h"

#include <ranges>
// temp include for testing
#include <QSerialPortInfo>

Interface::Interface() : connected_(false), hardwareTimestep_(0.005), commanding_(false)
{
  // @todo how to handle servo hardware factory ? probably need to pass arguments from front end?
  // for now hardcoded feetech type
  // setupServos(MotorType::Feetech);
  setupServos(MotorType::Simulated);
}

void Interface::connect(const std::string & device)
{
  connected_ = servoHardware_->openLink(device);
  if(connected_)
  {
    // Poll hardware and fill servo ids
    detectServos();
    // Initialize states and commands with initial state
    motorStates_.clear();
    controllerCommands_.clear();
    for(const auto & id : servoIds_)
    {
      motorStates_.emplace_back(getServoData(id));
      controllerCommands_.emplace_back(motorStates_.back());
      // Set first target to current position
      controllerCommands_.back().servoTargets.pos = motorStates_.back().position;
    }

    // Start hardware thread
    hardwareThread_ = std::jthread(
        [&](std::stop_token stopToken)
        {
          while(!stopToken.stop_requested())
          {
            auto start = std::chrono::high_resolution_clock::now();
            // Get servo states and update buffer
            {
              std::lock_guard<std::mutex> statesLock(statesMutex_);
              getServoData(motorStates_);
            }

            if(commanding_)
            {
              // Send commands to hardware
              std::lock_guard<std::mutex> commandsLock(commandsMutex_);
              // @fixme make a better data type for targets
              std::vector<int> ids;
              std::vector<double> targetsPos, targetsVel, targetsAcc;
              for(const auto & command : controllerCommands_)
              {
                ids.emplace_back(command.id);
                targetsPos.emplace_back(command.servoTargets.pos);
                targetsVel.emplace_back(command.servoTargets.vel);
                targetsAcc.emplace_back(command.servoTargets.acc);
              }
              setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
            }

            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(end - start).count();
            if(!elapsed > hardwareTimestep_)
            {
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(static_cast<unsigned int>(hardwareTimestep_ * 1000 - elapsed)));
            }
          }
        });
  }
}

void Interface::disconnect()
{
  if(hardwareThread_.joinable())
  {
    // Set the jthread stop token
    hardwareThread_.request_stop();
    hardwareThread_.join();
  }
  servoHardware_->closeLink();
  servoIds_.clear();
  connected_ = false;
}

void Interface::startController()
{
  if(controller_)
  {
    controller_->startControl();
    // Start controller thread
    controllerThread_ = std::jthread(
        [&](std::stop_token stopToken)
        {
          while(!stopToken.stop_requested())
          {
            auto start = std::chrono::high_resolution_clock::now();
            controller_->runControl();
            /* @note process in mc_udp is that the server holds a vector of sensors and a vector of commands (called
            encoders and encoders velocities but from qp output) each "execute" of the server calls the server send,
            which sends the sensor data buffer to the last connected client, then checks if any control data was
            received, and if so sends the control info
            */
            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(end - start).count();
            if(!elapsed > controller_->timestep())
            {
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(static_cast<unsigned int>(controller_->timestep() * 1000 - elapsed)));
            }
          }
        });
  }
}

void Interface::stopController()
{
  if(controller_)
  {
    // Stop controller thread and controller
    if(controllerThread_.joinable())
    {
      // Set the jthread stop token
      controllerThread_.request_stop();
      controllerThread_.join();
    }
    controller_->stopControl();
  }
}

void Interface::detectSerialPorts()
{
  // @fixme should be hardware->detect
  // for now using Qt explicitely
  for(const auto & info : QSerialPortInfo::availablePorts())
  {
    // @fixme explicitely adding /dev/ to path because qserial only returns port name
    serialPorts_.push_back("/dev/" + info.portName().toStdString());
  }
}

void Interface::detectServos()
{
  servoIds_ = servoHardware_->detectServos();
}

// @fixme this should set the state buffer instead of a direct hardware call
ServoInfo Interface::getServoData(int id)
{
  return servoHardware_->getServoData(id);
}

// @fixme this should set the state buffer instead of a direct hardware call
std::vector<ServoInfo> Interface::getServoData()
{
  std::vector<ServoInfo> buffer;
  for(const auto & id : servoIds_)
  {
    buffer.emplace_back(servoHardware_->getServoData(id));
  }
  return buffer;
}

// @fixme this should set the state buffer instead of a direct hardware call
void Interface::getServoData(std::vector<ServoInfo> & dataBuffer)
{
  for(auto & data : dataBuffer)
  {
    data = servoHardware_->getServoData(data.id);
  }
}

// @fixme this should set the command buffer instead of a direct hardware call
bool Interface::setServoTarget(int id, double targetPos, double targetVel, double targetAcc)
{
  ServoInfo::targets servoTarget{.pos = targetPos, .vel = targetVel, .acc = targetAcc};
  return servoHardware_->setServoTarget(id, servoTarget);
}

// @fixme this should set the command buffer instead of a direct hardware call
bool Interface::setServoTargets(const std::vector<int> & ids, const std::vector<double> & targetsPos)
{
  // If no target vel or acc passed, target zero
  // @fixme should we really target zero ? ok for acceleration but check hardware implementations of zero vel.
  // might make incoherent objective
  std::vector<double> targetsVel(ids.size()), targetsAcc(ids.size());

  return setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
}

// @fixme this should set the command buffer instead of a direct hardware call
bool Interface::setServoTargets(const std::vector<int> & ids,
                                const std::vector<double> & targetsPos,
                                const std::vector<double> & targetsVel)
{
  // If no target acc passed, target zero
  std::vector<double> targetsAcc(ids.size());

  return setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
}

// @fixme this should set the command buffer instead of a direct hardware call
bool Interface::setServoTargets(const std::vector<int> & ids,
                                const std::vector<double> & targetsPos,
                                const std::vector<double> & targetsVel,
                                const std::vector<double> & targetsAcc)
{
  // Build servo targets using std::views
  // Yes, a for loop is probably just as efficient as there are no complex operations
  // In c++23 this could be done easily using std::views::zip

  // create lazy range of the target vectors
  auto indices = std::views::iota(std::size_t{0}, ids.size());
  // lazy transform
  auto targetView = indices
                    | std::views::transform(
                        [&](size_t i) noexcept {
                          return ServoInfo::targets{.pos = targetsPos[i], .vel = targetsVel[i], .acc = targetsAcc[i]};
                        });

  std::vector<ServoInfo::targets> targets;
  targets.reserve(ids.size());
  // fill the targets vecor (use back inserter to avoid resizing the vector which initializes default values)
  std::ranges::copy(targetView, std::back_inserter(targets));

  return servoHardware_->setServoTargets(ids, targets);
}

// @fixme this should set the command buffer instead of a direct hardware call
bool Interface::setServoGains(int id, double P, double D, double I)
{
  return servoHardware_->setServoGains(id, P, D, I);
}

bool Interface::setServoOn(int id, bool activate)
{
  return servoHardware_->servoOn(id, activate);
}

void Interface::setupServos(MotorType type)
{
  servoHardware_ = setupServoType(type);
  // @fixme alert on nullptr
}

void Interface::setupController(ControllerType type, const ControllerConfig & config)
{
  controller_ = setupControllerType(type, config);
}
