/**
 * @file RouteRegistry.cpp
 * @brief Route registration implementation for the cloud backend.
 */

#include <cloud/presentation/routes/RouteRegistry.hpp>
#include <cloud/presentation/controllers/HomeController.hpp>
#include <cloud/presentation/controllers/HealthController.hpp>

#include <vix.hpp>

namespace cloud::presentation::routes
{
  void RouteRegistry::register_all(vix::App &app)
  {
    controllers::HomeController::register_routes(app);
    controllers::HealthController::register_routes(app);
  }
} // namespace cloud::presentation::routes
