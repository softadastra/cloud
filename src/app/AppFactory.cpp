/*
 * AppFactory.cpp
 */

#include "app/AppFactory.hpp"

#include <stdexcept>

#include <vix/log/Log.hpp>

#include "middlewares/MiddlewareRegistry.hpp"
#include "modules/auth/AuthRoutes.hpp"
#include "modules/health/HealthRoutes.hpp"
#include "modules/projects/ProjectRoutes.hpp"
#include "modules/agents/AgentRoutes.hpp"
#include "modules/runs/RunRoutes.hpp"
#include "modules/events/EventRoutes.hpp"
#include "modules/reports/ReportRoutes.hpp"

namespace softadastra::cloud::app
{
  vix::App AppFactory::create(const AppState &state)
  {
    if (!state.is_valid())
    {
      throw std::runtime_error(
          "cannot create Softadastra Cloud app from invalid AppState");
    }

    vix::log::info("creating Softadastra Cloud application");

    vix::App app;

    softadastra::cloud::middlewares::MiddlewareRegistry::install(
        app,
        state);

    softadastra::cloud::modules::health::HealthRoutes::register_routes(
        app,
        state);

    softadastra::cloud::modules::auth::AuthRoutes::register_routes(
        app,
        state);

    softadastra::cloud::modules::projects::ProjectRoutes::register_routes(
        app,
        state);

    softadastra::cloud::modules::agents::AgentRoutes::register_routes(
        app,
        state);

    softadastra::cloud::modules::runs::RunRoutes::register_routes(
        app,
        state);

    softadastra::cloud::modules::events::EventRoutes::register_routes(
        app,
        state);

    softadastra::cloud::modules::reports::ReportRoutes::register_routes(
        app,
        state);

    vix::log::info("Softadastra Cloud application created");

    return app;
  }

} // namespace softadastra::cloud::app
