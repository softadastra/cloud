/**
 *
 *  @file TokensModule.cpp
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
#include <tokens/TokensModule.hpp>
#include <tokens/controllers/TokensController.hpp>

#include <vix.hpp>

namespace cloud::tokens
{
  const char *TokensModule::name()
  {
    return "tokens";
  }

  void TokensModule::register_routes(vix::App &app)
  {
    controllers::TokensController::register_routes(app);
  }
} // namespace cloud::tokens
