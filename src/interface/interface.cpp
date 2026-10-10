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
      // First initialization calls hardware directly
      motorStates_.emplace_back(servoHardware_->getServoData(id));
      controllerCommands_.emplace_back(motorStates_.back());
      // Set first target to current position
      controllerCommands_.back().servoTargets.pos = motorStates_.back().position;
    }

    // @todo for now setting commanding_ to true directly but should be a front end option
    commanding_ = true;

    // Start hardware thread
    hardwareThread_ = std::jthread(
        [&](std::stop_token stopToken)
        {
          while(!stopToken.stop_requested())
          {
            auto start = std::chrono::high_resolution_clock::now();
            // Get hardware servo states and update buffer
            {
              // @fixme add hardware mutex lock just in case
              std::lock_guard<std::mutex> statesLock(statesMutex_);
              for(auto & data : motorStates_)
              {
                data = servoHardware_->getServoData(data.id);
              }
            }

            if(commanding_)
            {
              // Send commands to hardware
              // @fixme make a better data type for targets
              std::lock_guard<std::mutex> commandsLock(commandsMutex_);
              std::vector<int> ids;
              std::vector<ServoInfo::targets> targets;
              for(const auto & command : controllerCommands_)
              {
                ids.emplace_back(command.id);
                targets.emplace_back(command.servoTargets);
              }
              // @fixme add hardware mutex lock just in case
              servoHardware_->setServoTargets(ids, targets);
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
  commanding_ = false;
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
            controller_->runControl(statesMutex_, motorStates_, commandsMutex_, controllerCommands_);
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

ServoInfo Interface::getServoData(int id)
{
  std::lock_guard<std::mutex> statesLock(statesMutex_);
  for(const auto & data : motorStates_)
  {
    if(id == data.id)
    {
      return data;
    }
  }
  // if not found return default constructor
  return ServoInfo{};
}

std::vector<ServoInfo> Interface::getServoData()
{
  std::lock_guard<std::mutex> statesLock(statesMutex_);
  return motorStates_;
}

void Interface::getServoData(std::vector<ServoInfo> & dataBuffer)
{
  std::lock_guard<std::mutex> statesLock(statesMutex_);
  dataBuffer = motorStates_;
}

void Interface::setServoTarget(int id, double targetPos, double targetVel, double targetAcc)
{
  std::lock_guard<std::mutex> commandsLock(commandsMutex_);
  for(auto & command : controllerCommands_)
  {
    if(id == command.id)
    {
      command.servoTargets = ServoInfo::targets{.pos = targetPos, .vel = targetVel, .acc = targetAcc};
      break;
    }
  }
}

void Interface::setServoTargets(const std::vector<int> & ids, const std::vector<double> & targetsPos)
{
  // If no target vel or acc passed, target zero
  // @fixme should we really target zero ? ok for acceleration but check hardware implementations of zero vel.
  // might make incoherent objective
  std::vector<double> targetsVel(ids.size()), targetsAcc(ids.size());
  return setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
}

void Interface::setServoTargets(const std::vector<int> & ids,
                                const std::vector<double> & targetsPos,
                                const std::vector<double> & targetsVel)
{
  // If no target acc passed, target zero
  std::vector<double> targetsAcc(ids.size());
  return setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
}

void Interface::setServoTargets(const std::vector<int> & ids,
                                const std::vector<double> & targetsPos,
                                const std::vector<double> & targetsVel,
                                const std::vector<double> & targetsAcc)
{
  std::lock_guard<std::mutex> commandsLock(commandsMutex_);
  for(size_t i = 0; i < ids.size(); i++)
  {
    for(auto & command : controllerCommands_)
    {
      if(ids[i] == command.id)
      {
        command.servoTargets = ServoInfo::targets{.pos = targetsPos[i], .vel = targetsVel[i], .acc = targetsAcc[i]};
        // break out of the commands for when found
        break;
      }
    }
  }
}

void Interface::setServoGains(int id, double P, double D, double I)
{
  std::lock_guard<std::mutex> commandsLock(commandsMutex_);
  for(auto & command : controllerCommands_)
  {
    if(id == command.id)
    {
      command.servoGains = ServoInfo::gains{.kp = P, .kd = D, .ki = I};
      break;
    }
  }
}

bool Interface::setServoOn(int id, bool activate)
{
  // @fixme set on/off as part of command instead of direct hardware call
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
