/*
 * RunRoutes.cpp
 */

#include "modules/runs/RunRoutes.hpp"
#include "modules/runs/RunController.hpp"

namespace softadastra::cloud::modules::runs
{
  void RunRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.get(
        "/api/runs",
        [&state](vix::Request &req, vix::Response &res)
        {
          RunController::list(req, res, state);
        });

    app.post(
        "/api/runs",
        [&state](vix::Request &req, vix::Response &res)
        {
          RunController::create(req, res, state);
        });

    app.get(
        "/api/runs/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          RunController::get(req, res, state);
        });

    app.patch(
        "/api/runs/{id}/status",
        [&state](vix::Request &req, vix::Response &res)
        {
          RunController::update_status(req, res, state);
        });

    app.post(
        "/api/runs/{id}/finish",
        [&state](vix::Request &req, vix::Response &res)
        {
          RunController::finish(req, res, state);
        });

    app.del(
        "/api/runs/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          RunController::remove(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::runs
