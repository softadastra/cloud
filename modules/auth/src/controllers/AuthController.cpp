/**
 *
 *  @file AuthController.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Softadastra Cloud
 */
#include <auth/controllers/AuthController.hpp>

#include <vix.hpp>

namespace cloud::auth::controllers
{
  void AuthController::register_routes(vix::App &app)
  {
    app.get("/api/auth", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      res.json({
        "ok", true,
        "module", "auth",
        "message", "Auth module is available"
      }); });
  }
} // namespace cloud::auth::controllers
