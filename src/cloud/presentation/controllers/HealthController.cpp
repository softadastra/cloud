/**
 * @file HealthController.cpp
 * @brief Health route implementation for the cloud backend.
 */

#include <cloud/presentation/controllers/HealthController.hpp>

#include <vix.hpp>

namespace cloud::presentation::controllers
{
  void HealthController::register_routes(vix::App &app)
  {
    app.get("/health", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json(vix::json::o(
          "ok", true,
          "data", vix::json::o(
              "status", "ok",
              "service", "cloud")));
    });

    app.get("/api/health", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json(vix::json::o(
          "ok", true,
          "data", vix::json::o(
              "status", "ok",
              "service", "cloud",
              "api", true)));
    });
  }
} // namespace cloud::presentation::controllers
