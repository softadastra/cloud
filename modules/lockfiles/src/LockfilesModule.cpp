/**
 *
 *  @file LockfilesModule.cpp
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
#include <lockfiles/LockfilesModule.hpp>
#include <lockfiles/controllers/LockfilesController.hpp>

#include <vix.hpp>

namespace cloud::lockfiles
{
  const char *LockfilesModule::name()
  {
    return "lockfiles";
  }

  void LockfilesModule::register_routes(vix::App &app)
  {
    controllers::LockfilesController::register_routes(app);
  }
} // namespace cloud::lockfiles
