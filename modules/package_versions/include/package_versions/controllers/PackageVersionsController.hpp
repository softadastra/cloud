/**
 *
 *  @file PackageVersionsController.hpp
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
#ifndef cloud_package_versions_controller_hpp
#define cloud_package_versions_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::package_versions::controllers
{
  class PackageVersionsController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::package_versions::controllers

#endif // cloud_package_versions_controller_hpp
