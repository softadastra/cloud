/**
 *
 *  @file WorkspacesController.hpp
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
#ifndef cloud_workspaces_controller_hpp
#define cloud_workspaces_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::workspaces::controllers
{
  class WorkspacesController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::workspaces::controllers

#endif // cloud_workspaces_controller_hpp
