/**
 *
 *  @file RunRepository.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_REPOSITORY_HPP
#define SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/db/db.hpp>

#include "modules/runs/Run.hpp"

namespace softadastra::cloud::modules::runs
{
  /**
   * @brief Repository for reliability run persistence.
   *
   * RunRepository owns all SQL access for runs.
   *
   * It does not contain HTTP logic or business rules. Those belong to
   * RunController and RunService.
   */
  class RunRepository final
  {
  public:
    /**
     * @brief Creates a run repository.
     *
     * @param database Shared Vix database instance.
     */
    explicit RunRepository(
        std::shared_ptr<vix::db::Database> database);

    /**
     * @brief Creates a new reliability run.
     *
     * @param public_id Public run id.
     * @param project_id Internal project id.
     * @param agent_id Internal agent id. Can be 0 when unavailable.
     * @param status Initial run status.
     * @param target_url Target URL tested by the agent.
     * @param method HTTP method.
     * @param scenario Test scenario name.
     * @param started_at Run start timestamp.
     * @param now_ms Creation/update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool create(
        const std::string &public_id,
        std::int64_t project_id,
        std::int64_t agent_id,
        const std::string &status,
        const std::string &target_url,
        const std::string &method,
        const std::string &scenario,
        std::int64_t started_at,
        std::int64_t now_ms);

    /**
     * @brief Returns all runs.
     *
     * @param limit Max number of runs.
     * @param offset Offset for pagination.
     * @return Run list.
     */
    [[nodiscard]] std::vector<Run> all(
        int limit,
        int offset) const;

    /**
     * @brief Returns all runs for one project.
     *
     * @param project_public_id Public project id.
     * @param limit Max number of runs.
     * @param offset Offset for pagination.
     * @return Run list.
     */
    [[nodiscard]] std::vector<Run> all_for_project(
        const std::string &project_public_id,
        int limit,
        int offset) const;

    /**
     * @brief Returns all runs for one agent.
     *
     * @param agent_public_id Public agent id.
     * @param limit Max number of runs.
     * @param offset Offset for pagination.
     * @return Run list.
     */
    [[nodiscard]] std::vector<Run> all_for_agent(
        const std::string &agent_public_id,
        int limit,
        int offset) const;

    /**
     * @brief Finds a run by internal id.
     *
     * @param id Internal database id.
     * @return Run if found.
     */
    [[nodiscard]] std::optional<Run> find_by_id(
        std::int64_t id) const;

    /**
     * @brief Finds a run by public id.
     *
     * @param public_id Public run id.
     * @return Run if found.
     */
    [[nodiscard]] std::optional<Run> find_by_public_id(
        const std::string &public_id) const;

    /**
     * @brief Updates the status of a run.
     *
     * @param public_id Public run id.
     * @param status New status.
     * @param now_ms Update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool update_status(
        const std::string &public_id,
        const std::string &status,
        std::int64_t now_ms);

    /**
     * @brief Marks a run as finished.
     *
     * @param public_id Public run id.
     * @param status Final status.
     * @param finished_at Finish timestamp.
     * @param now_ms Update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool finish(
        const std::string &public_id,
        const std::string &status,
        std::int64_t finished_at,
        std::int64_t now_ms);

    /**
     * @brief Deletes a run by public id.
     *
     * @param public_id Public run id.
     * @return true on success.
     */
    [[nodiscard]] bool remove(
        const std::string &public_id);

    /**
     * @brief Counts all runs.
     *
     * @return Run count.
     */
    [[nodiscard]] std::int64_t count() const;

    /**
     * @brief Counts all runs for one project.
     *
     * @param project_public_id Public project id.
     * @return Run count.
     */
    [[nodiscard]] std::int64_t count_for_project(
        const std::string &project_public_id) const;

    /**
     * @brief Counts all runs for one agent.
     *
     * @param agent_public_id Public agent id.
     * @return Run count.
     */
    [[nodiscard]] std::int64_t count_for_agent(
        const std::string &agent_public_id) const;

    /**
     * @brief Counts runs by status.
     *
     * @param status Run status.
     * @return Matching run count.
     */
    [[nodiscard]] std::int64_t count_by_status(
        const std::string &status) const;

  private:
    std::shared_ptr<vix::db::Database> database_;

    [[nodiscard]] static Run run_from_current_row(
        const vix::db::Row &row);

    [[nodiscard]] static int normalize_limit(int limit) noexcept;
    [[nodiscard]] static int normalize_offset(int offset) noexcept;
  };

} // namespace softadastra::cloud::modules::runs

#endif // SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_REPOSITORY_HPP
