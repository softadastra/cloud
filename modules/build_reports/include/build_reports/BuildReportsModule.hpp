/**
 *
 *  @file BuildReportsModule.hpp
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
#ifndef cloud_build_reports_module_hpp
#define cloud_build_reports_module_hpp

namespace vix
{
  class App;
}

namespace cloud::build_reports
{
  class BuildReportsModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::build_reports

#endif // cloud_build_reports_module_hpp
