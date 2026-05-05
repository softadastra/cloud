/*
 * EventRoutes.cpp
 */

#include "modules/events/EventRoutes.hpp"
#include "modules/events/EventController.hpp"

namespace softadastra::cloud::modules::events
{
  void EventRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.get(
        "/api/events",
        [&state](vix::Request &req, vix::Response &res)
        {
          EventController::list(req, res, state);
        });

    app.get(
        "/api/events/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          EventController::get(req, res, state);
        });

    app.get(
        "/api/runs/{id}/events",
        [&state](vix::Request &req, vix::Response &res)
        {
          EventController::list_for_run(req, res, state);
        });

    app.post(
        "/api/runs/{id}/events",
        [&state](vix::Request &req, vix::Response &res)
        {
          EventController::create(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::events
