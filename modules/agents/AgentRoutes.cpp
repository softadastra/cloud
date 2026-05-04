/*
 * AgentRoutes.cpp
 */

#include "modules/agents/AgentRoutes.hpp"
#include "modules/agents/AgentController.hpp"

namespace softadastra::cloud::modules::agents
{
  void AgentRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.get(
        "/api/agents",
        [&state](vix::Request &req, vix::Response &res)
        {
          AgentController::list(req, res, state);
        });

    app.post(
        "/api/agents",
        [&state](vix::Request &req, vix::Response &res)
        {
          AgentController::create(req, res, state);
        });

    app.get(
        "/api/agents/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          AgentController::get(req, res, state);
        });

    app.post(
        "/api/agents/{id}/rotate-key",
        [&state](vix::Request &req, vix::Response &res)
        {
          AgentController::rotate_key(req, res, state);
        });

    app.del(
        "/api/agents/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          AgentController::remove(req, res, state);
        });

    app.get(
        "/api/agent/verify",
        [&state](vix::Request &req, vix::Response &res)
        {
          AgentController::verify(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::agents
