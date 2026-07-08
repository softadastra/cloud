/**
 *
 *  @file WorkspacesModule.cpp
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
#include <workspaces/WorkspacesModule.hpp>
#include <workspaces/controllers/WorkspacesController.hpp>

#include <vix.hpp>

namespace cloud::workspaces
{
  const char *WorkspacesModule::name()
  {
    return "workspaces";
  }

  void WorkspacesModule::register_routes(vix::App &app)
  {
    controllers::WorkspacesController::register_routes(app);
  }
} // namespace cloud::workspaces
