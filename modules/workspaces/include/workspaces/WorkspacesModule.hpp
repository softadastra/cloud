/**
 *
 *  @file WorkspacesModule.hpp
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
#ifndef cloud_workspaces_module_hpp
#define cloud_workspaces_module_hpp

namespace vix
{
  class App;
}

namespace cloud::workspaces
{
  class WorkspacesModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::workspaces

#endif // cloud_workspaces_module_hpp
