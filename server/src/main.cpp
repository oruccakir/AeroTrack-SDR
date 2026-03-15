#include "api_server.hpp"
#include "httplib.h"
#include "simulation_data.hpp"
#include "simulation_engine.hpp"

#include <csignal>
#include <iostream>

static httplib::Server *g_server = nullptr;

void signalHandler(int) {
  if (g_server) {
    g_server->stop();
  }
}

int main() {
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  aerotrack::SimulationData data;

  // Varsayılan hedef ekle
  data.targets.emplace_back(5000.0, 300.0, 10.0, 0.0, "SimHedef-1");

  aerotrack::SimulationEngine engine(data);
  aerotrack::ApiServer api(data, engine);

  httplib::Server svr;
  g_server = &svr;

  api.setupRoutes(svr);

  std::cout << "\n"
            << "╔══════════════════════════════════════════════════════════╗\n"
            << "║          AeroTrack-SDR  REST API Server                 ║\n"
            << "╚══════════════════════════════════════════════════════════╝\n"
            << "\n"
            << "  Listening on http://0.0.0.0:8080\n"
            << "  Health check: GET /api/health\n"
            << "\n"
            << "  Press Ctrl+C to stop.\n"
            << std::endl;

  svr.listen("0.0.0.0", 8080);

  engine.stop();
  std::cout << "\n[Server] Kapatıldı.\n" << std::endl;

  return 0;
}
