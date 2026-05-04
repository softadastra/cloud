/*
 * ProjectRoutes.cpp
 */

#include "modules/projects/ProjectRoutes.hpp"
#include "modules/projects/ProjectController.hpp"

namespace softadastra::cloud::modules::projects
{
  void ProjectRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.get(
        "/api/projects",
        [&state](vix::Request &req, vix::Response &res)
        {
          ProjectController::list(req, res, state);
        });

    app.post(
        "/api/projects",
        [&state](vix::Request &req, vix::Response &res)
        {
          ProjectController::create(req, res, state);
        });

    app.get(
        "/api/projects/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          ProjectController::get(req, res, state);
        });

    app.patch(
        "/api/projects/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          ProjectController::update(req, res, state);
        });

    app.del(
        "/api/projects/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          ProjectController::remove(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::projects
