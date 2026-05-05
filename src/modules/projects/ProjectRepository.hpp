/**
 *
 *  @file ProjectRepository.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_REPOSITORY_HPP
#define SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/db/db.hpp>

#include "modules/projects/Project.hpp"

namespace softadastra::cloud::modules::projects
{
  /**
   * @brief Repository for project persistence.
   *
   * ProjectRepository owns all SQL access for projects.
   *
   * It does not contain HTTP logic or business rules. Those belong to
   * ProjectController and ProjectService.
   */
  class ProjectRepository final
  {
  public:
    /**
     * @brief Creates a project repository.
     *
     * @param database Shared Vix database instance.
     */
    explicit ProjectRepository(
        std::shared_ptr<vix::db::Database> database);

    /**
     * @brief Creates a project.
     *
     * @param public_id Public project id.
     * @param user_id Owner user id. Can be 0 for MVP/system-owned projects.
     * @param name Project name.
     * @param slug Unique project slug.
     * @param description Optional project description.
     * @param base_url Optional target API base URL.
     * @param now_ms Creation/update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool create(
        const std::string &public_id,
        std::int64_t user_id,
        const std::string &name,
        const std::string &slug,
        const std::string &description,
        const std::string &base_url,
        std::int64_t now_ms);

    /**
     * @brief Returns all projects.
     *
     * @param limit Max number of projects.
     * @param offset Offset for pagination.
     * @return Project list.
     */
    [[nodiscard]] std::vector<Project> all(
        int limit,
        int offset) const;

    /**
     * @brief Finds a project by internal id.
     *
     * @param id Internal database id.
     * @return Project if found.
     */
    [[nodiscard]] std::optional<Project> find_by_id(
        std::int64_t id) const;

    /**
     * @brief Finds a project by public id.
     *
     * @param public_id Public project id.
     * @return Project if found.
     */
    [[nodiscard]] std::optional<Project> find_by_public_id(
        const std::string &public_id) const;

    /**
     * @brief Finds a project by slug.
     *
     * @param slug Project slug.
     * @return Project if found.
     */
    [[nodiscard]] std::optional<Project> find_by_slug(
        const std::string &slug) const;

    /**
     * @brief Returns true if a slug already exists.
     *
     * @param slug Project slug.
     * @return true when found.
     */
    [[nodiscard]] bool slug_exists(
        const std::string &slug) const;

    /**
     * @brief Updates a project.
     *
     * Empty strings are written as empty values. The service layer decides what
     * can be updated.
     *
     * @param public_id Public project id.
     * @param name Project name.
     * @param description Project description.
     * @param base_url Project base URL.
     * @param now_ms Update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool update(
        const std::string &public_id,
        const std::string &name,
        const std::string &description,
        const std::string &base_url,
        std::int64_t now_ms);

    /**
     * @brief Deletes a project by public id.
     *
     * @param public_id Public project id.
     * @return true on success.
     */
    [[nodiscard]] bool remove(
        const std::string &public_id);

    /**
     * @brief Counts all projects.
     *
     * @return Project count.
     */
    [[nodiscard]] std::int64_t count() const;

  private:
    std::shared_ptr<vix::db::Database> database_;

    [[nodiscard]] static Project project_from_current_row(
        const vix::db::ResultRow &row);

    [[nodiscard]] static int normalize_limit(int limit) noexcept;
    [[nodiscard]] static int normalize_offset(int offset) noexcept;
  };

} // namespace softadastra::cloud::modules::projects

#endif // SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_REPOSITORY_HPP
