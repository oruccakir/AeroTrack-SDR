#include "api_server.hpp"
#include "json.hpp"

using json = nlohmann::json;

namespace aerotrack {

ApiServer::ApiServer(SimulationData &data, SimulationEngine &engine)
    : data_(data), engine_(engine) {}

void ApiServer::setupRoutes(httplib::Server &svr) {
  svr.Get("/api/health",
          [this](const auto &req, auto &res) { handleGetHealth(req, res); });
  svr.Get("/api/config",
          [this](const auto &req, auto &res) { handleGetConfig(req, res); });
  svr.Put("/api/config",
          [this](const auto &req, auto &res) { handlePutConfig(req, res); });
  svr.Get("/api/derived",
          [this](const auto &req, auto &res) { handleGetDerived(req, res); });
  svr.Post("/api/simulation/start", [this](const auto &req, auto &res) {
    handleStartSimulation(req, res);
  });
  svr.Post("/api/simulation/stop", [this](const auto &req, auto &res) {
    handleStopSimulation(req, res);
  });
  svr.Get("/api/simulation/status",
          [this](const auto &req, auto &res) { handleGetStatus(req, res); });
  svr.Get("/api/targets",
          [this](const auto &req, auto &res) { handleGetTargets(req, res); });
  svr.Post("/api/targets",
           [this](const auto &req, auto &res) { handlePostTarget(req, res); });
  svr.Delete(R"(/api/targets/(\d+))",
             [this](const auto &req, auto &res) {
               handleDeleteTarget(req, res);
             });
  svr.Get("/api/data/waveform",
          [this](const auto &req, auto &res) { handleGetWaveform(req, res); });
  svr.Get("/api/data/range-doppler", [this](const auto &req, auto &res) {
    handleGetRangeDoppler(req, res);
  });
  svr.Get("/api/data/target-history", [this](const auto &req, auto &res) {
    handleGetTargetHistory(req, res);
  });
}

// --- Handlers ---

void ApiServer::handleGetHealth(const httplib::Request &,
                                httplib::Response &res) {
  json j = {{"status", "ok"}};
  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleGetConfig(const httplib::Request &,
                                httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);
  auto &c = data_.config;

  json j = {{"carrier_frequency_hz", c.carrier_frequency_hz},
            {"bandwidth_hz", c.bandwidth_hz},
            {"pulse_duration_s", c.pulse_duration_s},
            {"prf_hz", c.prf_hz},
            {"num_pulses_per_cpi", c.num_pulses_per_cpi},
            {"num_samples_per_pulse", c.num_samples_per_pulse},
            {"sampling_frequency_hz", c.sampling_frequency_hz},
            {"snr_db", c.snr_db},
            {"target_ip", c.target_ip},
            {"target_port", c.target_port},
            {"udp_enabled", data_.udp_enabled}};

  res.set_content(j.dump(), "application/json");
}

void ApiServer::handlePutConfig(const httplib::Request &req,
                                httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  if (data_.state == SimState::RUNNING) {
    res.status = 409;
    json err = {{"error", "Cannot modify config while simulation is running"}};
    res.set_content(err.dump(), "application/json");
    return;
  }

  try {
    auto j = json::parse(req.body);
    auto &c = data_.config;

    if (j.contains("carrier_frequency_hz"))
      c.carrier_frequency_hz = j["carrier_frequency_hz"].get<double>();
    if (j.contains("bandwidth_hz"))
      c.bandwidth_hz = j["bandwidth_hz"].get<double>();
    if (j.contains("pulse_duration_s"))
      c.pulse_duration_s = j["pulse_duration_s"].get<double>();
    if (j.contains("prf_hz"))
      c.prf_hz = j["prf_hz"].get<double>();
    if (j.contains("num_pulses_per_cpi"))
      c.num_pulses_per_cpi = j["num_pulses_per_cpi"].get<uint32_t>();
    if (j.contains("num_samples_per_pulse"))
      c.num_samples_per_pulse = j["num_samples_per_pulse"].get<uint32_t>();
    if (j.contains("sampling_frequency_hz"))
      c.sampling_frequency_hz = j["sampling_frequency_hz"].get<double>();
    if (j.contains("snr_db"))
      c.snr_db = j["snr_db"].get<double>();
    if (j.contains("target_ip"))
      c.target_ip = j["target_ip"].get<std::string>();
    if (j.contains("target_port"))
      c.target_port = j["target_port"].get<uint16_t>();
    if (j.contains("udp_enabled"))
      data_.udp_enabled = j["udp_enabled"].get<bool>();

    // Reset state to IDLE after config change
    data_.state = SimState::IDLE;

    json resp = {{"status", "ok"}};
    res.set_content(resp.dump(), "application/json");
  } catch (const std::exception &e) {
    res.status = 400;
    json err = {{"error", std::string("Invalid JSON: ") + e.what()}};
    res.set_content(err.dump(), "application/json");
  }
}

void ApiServer::handleGetDerived(const httplib::Request &,
                                 httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);
  auto &c = data_.config;

  json j = {{"wavelength_m", c.wavelength()},
            {"chirp_rate", c.chirp_rate()},
            {"pri_s", c.pri_s()},
            {"max_unambiguous_range_m", c.max_unambiguous_range()},
            {"max_unambiguous_velocity_ms", c.max_unambiguous_velocity()},
            {"range_resolution_m", c.range_resolution()},
            {"velocity_resolution_ms", c.velocity_resolution()},
            {"packet_size_bytes",
             24 + static_cast<int>(c.num_samples_per_pulse) * 8},
            {"data_rate_mbps",
             (24 + c.num_samples_per_pulse * 8) * c.prf_hz / 1e6}};

  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleStartSimulation(const httplib::Request &req,
                                      httplib::Response &res) {
  {
    std::lock_guard<std::mutex> lock(data_.mtx);
    if (data_.state == SimState::RUNNING) {
      res.status = 409;
      json err = {{"error", "Simulation is already running"}};
      res.set_content(err.dump(), "application/json");
      return;
    }

    // Parse optional cpi_count
    if (!req.body.empty()) {
      try {
        auto j = json::parse(req.body);
        if (j.contains("cpi_count")) {
          data_.max_cpi_count = j["cpi_count"].get<uint32_t>();
        }
      } catch (...) {
        // ignore parse errors, use defaults
      }
    } else {
      data_.max_cpi_count = 0;
    }
  }

  engine_.start();

  json j = {{"status", "started"}};
  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleStopSimulation(const httplib::Request &,
                                     httplib::Response &res) {
  engine_.stop();

  json j = {{"status", "stopped"}};
  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleGetStatus(const httplib::Request &,
                                httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  json j = {{"state", simStateToString(data_.state)},
            {"current_cpi", data_.current_cpi},
            {"total_packets", data_.total_packets},
            {"elapsed_s", data_.elapsed_s},
            {"throughput_mbps", data_.throughput_mbps},
            {"max_cpi_count", data_.max_cpi_count}};

  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleGetTargets(const httplib::Request &,
                                 httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  json arr = json::array();
  for (const auto &t : data_.targets) {
    arr.push_back({{"range_m", t.range_m},
                   {"velocity_kmh", t.velocity_kmh},
                   {"rcs_m2", t.rcs_m2},
                   {"azimuth_deg", t.azimuth_deg},
                   {"name", t.name}});
  }

  res.set_content(arr.dump(), "application/json");
}

void ApiServer::handlePostTarget(const httplib::Request &req,
                                 httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  if (data_.state == SimState::RUNNING) {
    res.status = 409;
    json err = {{"error", "Cannot modify targets while simulation is running"}};
    res.set_content(err.dump(), "application/json");
    return;
  }

  try {
    auto j = json::parse(req.body);
    Target t(j.value("range_m", 5000.0), j.value("velocity_kmh", 300.0),
             j.value("rcs_m2", 10.0), j.value("azimuth_deg", 0.0),
             j.value("name", "Target-" + std::to_string(data_.targets.size() + 1)));
    data_.targets.push_back(t);

    // Reset to IDLE
    data_.state = SimState::IDLE;

    json resp = {{"status", "ok"}, {"index", data_.targets.size() - 1}};
    res.set_content(resp.dump(), "application/json");
  } catch (const std::exception &e) {
    res.status = 400;
    json err = {{"error", std::string("Invalid JSON: ") + e.what()}};
    res.set_content(err.dump(), "application/json");
  }
}

void ApiServer::handleDeleteTarget(const httplib::Request &req,
                                   httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  if (data_.state == SimState::RUNNING) {
    res.status = 409;
    json err = {{"error", "Cannot modify targets while simulation is running"}};
    res.set_content(err.dump(), "application/json");
    return;
  }

  size_t index = std::stoul(req.matches[1]);

  if (index >= data_.targets.size()) {
    res.status = 404;
    json err = {{"error", "Target not found"}};
    res.set_content(err.dump(), "application/json");
    return;
  }

  data_.targets.erase(data_.targets.begin() + static_cast<long>(index));
  data_.state = SimState::IDLE;

  json j = {{"status", "ok"}};
  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleGetWaveform(const httplib::Request &,
                                  httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  json j;
  j["sample_count"] = data_.latest_iq_waveform.size();

  json i_arr = json::array();
  json q_arr = json::array();

  for (const auto &s : data_.latest_iq_waveform) {
    i_arr.push_back(s.real());
    q_arr.push_back(s.imag());
  }

  j["i"] = i_arr;
  j["q"] = q_arr;

  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleGetRangeDoppler(const httplib::Request &,
                                      httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  json j;
  j["range_bins"] = data_.rd_range_bins;
  j["doppler_bins"] = data_.rd_doppler_bins;
  j["data"] = data_.range_doppler_map;

  res.set_content(j.dump(), "application/json");
}

void ApiServer::handleGetTargetHistory(const httplib::Request &,
                                       httplib::Response &res) {
  std::lock_guard<std::mutex> lock(data_.mtx);

  json arr = json::array();
  for (const auto &snap : data_.target_history) {
    json targets_arr = json::array();
    for (const auto &t : snap.targets) {
      targets_arr.push_back({{"range_m", t.range_m},
                             {"velocity_kmh", t.velocity_kmh},
                             {"rcs_m2", t.rcs_m2},
                             {"azimuth_deg", t.azimuth_deg},
                             {"name", t.name}});
    }
    arr.push_back({{"cpi_index", snap.cpi_index},
                   {"elapsed_s", snap.elapsed_s},
                   {"targets", targets_arr}});
  }

  res.set_content(arr.dump(), "application/json");
}

} // namespace aerotrack
