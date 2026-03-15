#pragma once

#include "httplib.h"
#include "simulation_data.hpp"
#include "simulation_engine.hpp"

namespace aerotrack {

/**
 * @brief REST API sunucusu.
 */
class ApiServer {
public:
  ApiServer(SimulationData &data, SimulationEngine &engine);

  void setupRoutes(httplib::Server &svr);

private:
  SimulationData &data_;
  SimulationEngine &engine_;

  // Route handlers
  void handleGetHealth(const httplib::Request &req, httplib::Response &res);
  void handleGetConfig(const httplib::Request &req, httplib::Response &res);
  void handlePutConfig(const httplib::Request &req, httplib::Response &res);
  void handleGetDerived(const httplib::Request &req, httplib::Response &res);
  void handleStartSimulation(const httplib::Request &req,
                             httplib::Response &res);
  void handleStopSimulation(const httplib::Request &req,
                            httplib::Response &res);
  void handleGetStatus(const httplib::Request &req, httplib::Response &res);
  void handleGetTargets(const httplib::Request &req, httplib::Response &res);
  void handlePostTarget(const httplib::Request &req, httplib::Response &res);
  void handleDeleteTarget(const httplib::Request &req, httplib::Response &res);
  void handleGetWaveform(const httplib::Request &req, httplib::Response &res);
  void handleGetRangeDoppler(const httplib::Request &req,
                             httplib::Response &res);
  void handleGetTargetHistory(const httplib::Request &req,
                              httplib::Response &res);
};

} // namespace aerotrack
