/*
 * AuthRoutes.cpp
 */

#include "modules/auth/AuthRoutes.hpp"
#include "modules/auth/AuthController.hpp"

namespace softadastra::cloud::modules::auth
{
  void AuthRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.post(
        "/api/auth/register",
        [&state](vix::Request &req, vix::Response &res)
        {
          AuthController::register_user(req, res, state);
        });

    app.post(
        "/api/auth/login",
        [&state](vix::Request &req, vix::Response &res)
        {
          AuthController::login(req, res, state);
        });

    app.get(
        "/api/auth/me",
        [&state](vix::Request &req, vix::Response &res)
        {
          AuthController::me(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::auth
