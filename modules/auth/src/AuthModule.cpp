/**
 *
 *  @file AuthModule.cpp
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
#include <auth/AuthModule.hpp>
#include <auth/controllers/AuthController.hpp>

#include <vix.hpp>

namespace cloud::auth
{
  const char *AuthModule::name()
  {
    return "auth";
  }

  void AuthModule::register_routes(vix::App &app)
  {
    controllers::AuthController::register_routes(app);
  }
} // namespace cloud::auth
