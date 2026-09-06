#pragma once

#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "servos/servos.h"

class SimulatedServos : public ServoHardware
{
public:
  SimulatedServos();

  bool openLink(const std::string & device) override;
  bool closeLink() override;
  std::vector<int> detectServos() override;

  ServoInfo getServoData(const int & id) override;
  bool servoOn(const int & id, const bool & activate) override;
  bool setServoGains(const int & id, const double & P, const double & D, const double & I) override;
  bool setServoTarget(const int & id, const ServoInfo::targets & target) override;
  bool setServoTargets(const std::vector<int> & ids, std::vector<ServoInfo::targets> targets) override;

private:
  // Random generator for noise
  std::mt19937 randGen_;
  double randomNoise(double maxNoise);

  // Stored dummy servos
  std::map<int, ServoInfo> dummyServos_;

  // Threading stuff to simulate dummy servos hardware update
  mutable std::mutex servoDataMutex_;
  std::jthread servoDataUpdater_;
  void startServoUpdateThread();
  void stopServoUpdateThread();

  void simulateDummyServoUpdate(ServoInfo & dummyData);
};
