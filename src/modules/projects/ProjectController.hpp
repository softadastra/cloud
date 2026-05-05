/**
 *
 *  @file ProjectController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::projects
{
  /**
   * @brief HTTP controller for project endpoints.
   *
   * ProjectController handles only HTTP concerns:
   * - read path/query parameters
   * - read JSON request body
   * - call ProjectService
   * - return standardized JSON responses
   *
   * Business rules belong in ProjectService.
   * SQL belongs in ProjectRepository.
   */
  class ProjectController final
  {
  public:
    /**
     * @brief Creates a project.
     *
     * Expected JSON body:
     * {
     *   "name": "My API",
     *   "description": "Optional description",
     *   "base_url": "https://api.example.com"
     * }
     */
    static void create(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Lists projects.
     *
     * Query params:
     * - limit
     * - offset
     */
    static void list(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Gets one project by public id.
     *
     * Path:
     * /api/projects/{id}
     */
    static void get(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Updates one project by public id.
     *
     * Path:
     * /api/projects/{id}
     */
    static void update(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Deletes one project by public id.
     *
     * Path:
     * /api/projects/{id}
     */
    static void remove(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    ProjectController() = delete;
  };

} // namespace softadastra::cloud::modules::projects

#endif // SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_CONTROLLER_HPP
