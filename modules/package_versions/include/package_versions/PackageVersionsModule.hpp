/**
 *
 *  @file PackageVersionsModule.hpp
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
#ifndef cloud_package_versions_module_hpp
#define cloud_package_versions_module_hpp

namespace vix
{
  class App;
}

namespace cloud::package_versions
{
  class PackageVersionsModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::package_versions

#endif // cloud_package_versions_module_hpp
