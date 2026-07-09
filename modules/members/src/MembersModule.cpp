/**
 *
 *  @file MembersModule.cpp
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
#include <members/MembersModule.hpp>
#include <members/controllers/MembersController.hpp>

#include <vix.hpp>

namespace cloud::members
{
  const char *MembersModule::name()
  {
    return "members";
  }

  void MembersModule::register_routes(vix::App &app)
  {
    controllers::MembersController::register_routes(app);
  }
} // namespace cloud::members
