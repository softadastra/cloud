/**
 *
 *  @file PackagesModule.cpp
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
#include <packages/PackagesModule.hpp>
#include <packages/controllers/PackagesController.hpp>

#include <vix.hpp>

namespace cloud::packages
{
  const char *PackagesModule::name()
  {
    return "packages";
  }

  void PackagesModule::register_routes(vix::App &app)
  {
    controllers::PackagesController::register_routes(app);
  }
} // namespace cloud::packages
