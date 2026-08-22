#include "servos/simulated.h"

#include <random>

SimulatedServos::SimulatedServos()
{
  std::random_device rd;
  randGen_ = std::mt19937(rd());
  for(int i = 1; i < 7; i++)
  {
    ServoInfo dummy{.id{i},
                    .voltage{7.8},
                    .current{0.},
                    .kp{10.},
                    .ki{0.},
                    .kd{0.},
                    .position{0.},
                    .torque{0.},
                    .temperature{30.},
                    .torqueMode{false}};
    dummyServos_.emplace(i, dummy);
  }
}

bool SimulatedServos::openLink(const std::string & device)
{
  startServoUpdateThread();
  return true;
}

bool SimulatedServos::closeLink()
{
  stopServoUpdateThread();
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

ServoInfo SimulatedServos::getServoData(int id)
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

          std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
