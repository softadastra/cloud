/*
 * HealthRoutes.cpp
 */

#include "modules/health/HealthRoutes.hpp"
#include "modules/health/HealthController.hpp"

namespace softadastra::cloud::modules::health
{
  void HealthRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.get(
        "/health",
        [&state](vix::Request &req, vix::Response &res)
        {
          HealthController::health(req, res, state);
        });

    app.get(
        "/ready",
        [&state](vix::Request &req, vix::Response &res)
        {
          HealthController::readiness(req, res, state);
        });

    app.get(
        "/api/health",
        [&state](vix::Request &req, vix::Response &res)
        {
          HealthController::health(req, res, state);
        });

    app.get(
        "/api/ready",
        [&state](vix::Request &req, vix::Response &res)
        {
          HealthController::readiness(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::health
