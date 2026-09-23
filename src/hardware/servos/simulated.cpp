#include "servos/simulated.h"

#include <random>

SimulatedServos::SimulatedServos()
{
  std::random_device rd;
  randGen_ = std::mt19937(rd());
}

bool SimulatedServos::openLink(const std::string & device)
{
  for(int i = 1; i < 7; i++)
  {
    // Only initializing non default arguments
    ServoInfo dummy{.id = i, .voltage = 7.8, .temperature = 30., .servoGains = {.kp = 10., .kd = 6.32}};
    dummyServos_.emplace(i, dummy);
  }
  startServoUpdateThread();
  return true;
}

bool SimulatedServos::closeLink()
{
  stopServoUpdateThread();
  dummyServos_.clear();
  return true;
}

std::vector<int> SimulatedServos::detectServos()
{
  std::vector<int> servoIds;
  std::lock_guard<std::mutex> lock(servoDataMutex_);
  for(const auto & [_, servo] : dummyServos_)
  {
    servoIds.push_back(servo.id);
  }
  return servoIds;
}

ServoInfo SimulatedServos::getServoData(const int & id)
{
  std::lock_guard<std::mutex> lock(servoDataMutex_);
  return dummyServos_.at(id);
}

double SimulatedServos::randomNoise(double maxNoise)
{
  std::uniform_real_distribution<double> dist(-maxNoise, maxNoise);
  return dist(randGen_);
}

void SimulatedServos::simulateDummyServoUpdate(ServoInfo & dummyData)
{
  dummyData.current = 0.01 + randomNoise(0.005);
  dummyData.voltage = 7.8 + randomNoise(0.2);
  dummyData.temperature = 30. + randomNoise(1.);
  // assuming the 100ms timestep of the thread has passed, update simulated state
  auto posError = dummyData.servoTargets.pos - dummyData.position;
  auto velError = dummyData.servoTargets.vel - dummyData.velocity;
  auto commandAcc =
      dummyData.servoTargets.acc + dummyData.servoGains.kp * posError + dummyData.servoGains.kd * velError;

  // simulating motion only if simulated servo on
  if(dummyData.torqueEnabled)
  {
    dummyData.velocity += commandAcc * 0.01;
    dummyData.position += dummyData.velocity * 0.01; // semi implicit euler
  }
  else
  {
    dummyData.velocity = 0.;
  }
}

void SimulatedServos::startServoUpdateThread()
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
          std::unique_lock<std::mutex> lock(servoDataMutex_);
          for(auto & [_, servoData] : dummyServos_)
          {
            simulateDummyServoUpdate(servoData);
          }
          lock.unlock();

          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
}

void SimulatedServos::stopServoUpdateThread()
{
  if(!servoDataUpdater_.joinable())
  {
    return;
  }
  // Set the jthread stop token
  servoDataUpdater_.request_stop();
  servoDataUpdater_.join();
}

bool SimulatedServos::servoOn(const int & id, const bool & activate)
{
  std::lock_guard<std::mutex> lock(servoDataMutex_);
  if(dummyServos_.contains(id))
  {
    dummyServos_.at(id).torqueEnabled = activate;
    return true;
  }
  return false;
}

bool SimulatedServos::setServoGains(const int & id, const double & P, const double & D, const double & I)
{
  std::lock_guard<std::mutex> lock(servoDataMutex_);
  if(dummyServos_.contains(id))
  {
    dummyServos_.at(id).servoGains.kp = P;
    dummyServos_.at(id).servoGains.kd = D;
    dummyServos_.at(id).servoGains.ki = I;
    return true;
  }
  return false;
}

bool SimulatedServos::setServoTarget(const int & id, const ServoInfo::targets & target)
{
  std::lock_guard<std::mutex> lock(servoDataMutex_);
  if(dummyServos_.contains(id))
  {
    dummyServos_.at(id).servoTargets = target;
    return true;
  }
  return false;
}

bool SimulatedServos::setServoTargets(const std::vector<int> & ids, std::vector<ServoInfo::targets> targets)
{
  std::lock_guard<std::mutex> lock(servoDataMutex_);
  for(size_t i = 0; i < ids.size(); i++)
  {
    if(!dummyServos_.contains(ids[i]))
    {
      return false;
    }
    dummyServos_.at(ids[i]).servoTargets = targets[i];
  }
  return true;
}
