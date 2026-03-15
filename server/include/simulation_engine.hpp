#pragma once

#include "simulation_data.hpp"
#include <atomic>
#include <thread>

namespace aerotrack {

/**
 * @brief Arka plan thread'inde simülasyon çalıştırır.
 */
class SimulationEngine {
public:
  explicit SimulationEngine(SimulationData &data);
  ~SimulationEngine();

  void start();
  void stop();
  bool isRunning() const { return running_.load(); }

private:
  void run();

  SimulationData &data_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stop_requested_{false};
  std::thread thread_;
};

} // namespace aerotrack
