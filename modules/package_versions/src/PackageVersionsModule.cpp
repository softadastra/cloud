/**
 *
 *  @file PackageVersionsModule.cpp
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
#include <package_versions/PackageVersionsModule.hpp>
#include <package_versions/controllers/PackageVersionsController.hpp>

#include <vix.hpp>

namespace cloud::package_versions
{
  const char *PackageVersionsModule::name()
  {
    return "package_versions";
  }

  void PackageVersionsModule::register_routes(vix::App &app)
  {
    controllers::PackageVersionsController::register_routes(app);
  }
} // namespace cloud::package_versions
