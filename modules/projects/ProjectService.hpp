/**
 *
 *  @file ProjectService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_SERVICE_HPP

#include <string>
#include <vector>

#include "app/AppState.hpp"
#include "modules/projects/Project.hpp"
#include "modules/projects/ProjectRepository.hpp"

namespace softadastra::cloud::modules::projects
{
  /**
   * @brief Generic result returned by project operations.
   */
  struct ProjectResult
  {
    bool success{false};
    int status{400};

    std::string error{"project_error"};
    std::string message{};

    Project project{};
  };

  /**
   * @brief List result returned by project listing.
   */
  struct ProjectListResult
  {
    bool success{false};
    int status{400};

    std::string error{"project_error"};
    std::string message{};

    std::vector<Project> projects{};
    std::int64_t total{0};
    int limit{20};
    int offset{0};
  };

  /**
   * @brief Project business service.
   *
   * ProjectService owns the business rules for projects:
   * - validating project input
   * - generating public ids
   * - generating slugs
   * - checking conflicts
   * - delegating persistence to ProjectRepository
   *
   * It does not read Vix Request and does not write Vix Response.
   */
  class ProjectService final
  {
  public:
    /**
     * @brief Creates a project service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit ProjectService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Creates a new project.
     *
     * @param name Project name.
     * @param description Project description.
     * @param base_url Target API base URL.
     * @return Project result.
     */
    [[nodiscard]] ProjectResult create_project(
        const std::string &name,
        const std::string &description,
        const std::string &base_url);

    /**
     * @brief Lists projects with pagination.
     *
     * @param limit Max number of projects.
     * @param offset Pagination offset.
     * @return Project list result.
     */
    [[nodiscard]] ProjectListResult list_projects(
        int limit,
        int offset);

    /**
     * @brief Gets one project by public id.
     *
     * @param public_id Public project id.
     * @return Project result.
     */
    [[nodiscard]] ProjectResult get_project(
        const std::string &public_id);

    /**
     * @brief Updates a project.
     *
     * @param public_id Public project id.
     * @param name Project name.
     * @param description Project description.
     * @param base_url Target API base URL.
     * @return Project result.
     */
    [[nodiscard]] ProjectResult update_project(
        const std::string &public_id,
        const std::string &name,
        const std::string &description,
        const std::string &base_url);

    /**
     * @brief Deletes a project.
     *
     * @param public_id Public project id.
     * @return Project result.
     */
    [[nodiscard]] ProjectResult delete_project(
        const std::string &public_id);

  private:
    const softadastra::cloud::app::AppState &state_;
    ProjectRepository repository_;

    [[nodiscard]] static std::string make_slug(
        const std::string &name);

    [[nodiscard]] std::string make_unique_slug(
        const std::string &name) const;

    [[nodiscard]] static bool is_valid_name(
        const std::string &name) noexcept;

    [[nodiscard]] static bool is_valid_base_url(
        const std::string &base_url) noexcept;

    [[nodiscard]] static ProjectResult validation_error(
        std::string message);

    [[nodiscard]] static ProjectResult not_found(
        std::string message);

    [[nodiscard]] static ProjectResult conflict(
        std::string message);

    [[nodiscard]] static ProjectResult internal_error(
        std::string message);

    [[nodiscard]] static ProjectListResult list_error(
        int status,
        std::string error,
        std::string message);
  };

} // namespace softadastra::cloud::modules::projects

#endif // SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_SERVICE_HPP
