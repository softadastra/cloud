/**
 *
 *  @file ProjectsController.hpp
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
#ifndef cloud_projects_controller_hpp
#define cloud_projects_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::projects::controllers
{
  class ProjectsController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::projects::controllers

#endif // cloud_projects_controller_hpp
