#include "interface.h"

#include <ranges>
// temp include for testing
#include <QSerialPortInfo>

Interface::Interface() : connected_(false)
{
  // @todo how to handle servo hardware factory ? probably need to pass arguments from front end?
  // for now hardcoded feetech type
  // setupServos(MotorType::Feetech);
  setupServos(MotorType::Simulated);
}

void Interface::connect(const std::string & device)
{
  connected_ = servoHardware_->openLink(device);
}

void Interface::disconnect()
{
  servoHardware_->closeLink();
  servoIds_.clear();
  connected_ = false;
}

void Interface::startController()
{
  // start and store shared ptr of controller instance
  // should start controller thread connected to hardware command and sensor feedback?
  // how to handle if controller cuts on its own gracefully?
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
  return servoHardware_->getServoData(id);
}

bool Interface::setServoTarget(int id, double targetPos, double targetVel, double targetAcc)
{
  ServoInfo::targets servoTarget{.pos = targetPos, .vel = targetVel, .acc = targetAcc};
  return servoHardware_->setServoTarget(id, servoTarget);
}

bool Interface::setServoTargets(const std::vector<int> & ids, const std::vector<double> & targetsPos)
{
  // If no target vel or acc passed, target zero
  // @fixme should we really target zero ? ok for acceleration but check hardware implementations of zero vel.
  // might make incoherent objective
  std::vector<double> targetsVel(ids.size()), targetsAcc(ids.size());

  return setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
}

bool Interface::setServoTargets(const std::vector<int> & ids,
                                const std::vector<double> & targetsPos,
                                const std::vector<double> & targetsVel)
{
  // If no target acc passed, target zero
  std::vector<double> targetsAcc(ids.size());

  return setServoTargets(ids, targetsPos, targetsVel, targetsAcc);
}

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

void Interface::setupController(ControllerType type)
{
  controller_ = setupControllerType(type);
  // @fixme alert on nullptr
}
