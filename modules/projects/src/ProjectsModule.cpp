/**
 *
 *  @file ProjectsModule.cpp
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
#include <projects/ProjectsModule.hpp>
#include <projects/controllers/ProjectsController.hpp>

#include <vix.hpp>

namespace cloud::projects
{
  const char *ProjectsModule::name()
  {
    return "projects";
  }

  void ProjectsModule::register_routes(vix::App &app)
  {
    controllers::ProjectsController::register_routes(app);
  }
} // namespace cloud::projects
