/**
 *
 *  @file ProjectRoutes.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra/cloud
 *
 *  Licensed under the Apache License, Version 2.0.
 *
 *  Softadastra Cloud
 *
 */

#ifndef SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_ROUTES_HPP
#define SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_ROUTES_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::projects
{
  /**
   * @brief Registers project routes.
   *
   * ProjectRoutes only declares HTTP paths and forwards requests to
   * ProjectController.
   */
  class ProjectRoutes final
  {
  public:
    /**
     * @brief Registers project routes on the Vix app.
     *
     * @param app Vix application.
     * @param state Shared application state.
     */
    static void register_routes(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

  private:
    ProjectRoutes() = delete;
  };

} // namespace softadastra::cloud::modules::projects

#endif // SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_ROUTES_HPP
