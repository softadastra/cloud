/**
 *
 *  @file RunService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_SERVICE_HPP

#include <string>
#include <vector>

#include "app/AppState.hpp"
#include "modules/agents/AgentRepository.hpp"
#include "modules/projects/ProjectRepository.hpp"
#include "modules/runs/Run.hpp"
#include "modules/runs/RunRepository.hpp"

namespace softadastra::cloud::modules::runs
{
  /**
   * @brief Result returned by run operations.
   */
  struct RunResult
  {
    bool success{false};
    int status{400};

    std::string error{"run_error"};
    std::string message{};

    Run run{};
  };

  /**
   * @brief List result returned by run listing.
   */
  struct RunListResult
  {
    bool success{false};
    int status{400};

    std::string error{"run_error"};
    std::string message{};

    std::vector<Run> runs{};
    std::int64_t total{0};
    int limit{20};
    int offset{0};
  };

  /**
   * @brief Run business service.
   *
   * RunService owns the business rules for reliability test runs:
   * - validating run input
   * - checking project existence
   * - optionally checking agent existence
   * - creating runs
   * - finishing runs
   * - filtering runs by project or agent
   *
   * It does not read Vix Request and does not write Vix Response.
   */
  class RunService final
  {
  public:
    /**
     * @brief Creates a run service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit RunService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Creates a new reliability run.
     *
     * @param project_public_id Public project id.
     * @param agent_public_id Optional public agent id.
     * @param target_url Target URL tested by the agent.
     * @param method HTTP method.
     * @param scenario Reliability scenario name.
     * @return Run result.
     */
    [[nodiscard]] RunResult create_run(
        const std::string &project_public_id,
        const std::string &agent_public_id,
        const std::string &target_url,
        const std::string &method,
        const std::string &scenario);

    /**
     * @brief Lists all runs.
     *
     * @param limit Max number of runs.
     * @param offset Pagination offset.
     * @return Run list result.
     */
    [[nodiscard]] RunListResult list_runs(
        int limit,
        int offset);

    /**
     * @brief Lists runs for one project.
     *
     * @param project_public_id Public project id.
     * @param limit Max number of runs.
     * @param offset Pagination offset.
     * @return Run list result.
     */
    [[nodiscard]] RunListResult list_runs_for_project(
        const std::string &project_public_id,
        int limit,
        int offset);

    /**
     * @brief Lists runs for one agent.
     *
     * @param agent_public_id Public agent id.
     * @param limit Max number of runs.
     * @param offset Pagination offset.
     * @return Run list result.
     */
    [[nodiscard]] RunListResult list_runs_for_agent(
        const std::string &agent_public_id,
        int limit,
        int offset);

    /**
     * @brief Gets one run by public id.
     *
     * @param public_id Public run id.
     * @return Run result.
     */
    [[nodiscard]] RunResult get_run(
        const std::string &public_id);

    /**
     * @brief Updates one run status.
     *
     * @param public_id Public run id.
     * @param status New status.
     * @return Run result.
     */
    [[nodiscard]] RunResult update_status(
        const std::string &public_id,
        const std::string &status);

    /**
     * @brief Finishes one run.
     *
     * @param public_id Public run id.
     * @param status Final status.
     * @return Run result.
     */
    [[nodiscard]] RunResult finish_run(
        const std::string &public_id,
        const std::string &status);

    /**
     * @brief Deletes one run.
     *
     * @param public_id Public run id.
     * @return Run result.
     */
    [[nodiscard]] RunResult delete_run(
        const std::string &public_id);

  private:
    const softadastra::cloud::app::AppState &state_;
    RunRepository repository_;
    softadastra::cloud::modules::projects::ProjectRepository project_repository_;
    softadastra::cloud::modules::agents::AgentRepository agent_repository_;

    [[nodiscard]] static bool is_valid_target_url(
        const std::string &target_url) noexcept;

    [[nodiscard]] static bool is_valid_method(
        const std::string &method) noexcept;

    [[nodiscard]] static bool is_valid_scenario(
        const std::string &scenario) noexcept;

    [[nodiscard]] static bool is_valid_status(
        const std::string &status) noexcept;

    [[nodiscard]] static bool is_final_status(
        const std::string &status) noexcept;

    [[nodiscard]] static std::string normalize_method(
        const std::string &method);

    [[nodiscard]] static RunResult validation_error(
        std::string message);

    [[nodiscard]] static RunResult not_found(
        std::string message);

    [[nodiscard]] static RunResult internal_error(
        std::string message);

    [[nodiscard]] static RunListResult list_error(
        int status,
        std::string error,
        std::string message);
  };

} // namespace softadastra::cloud::modules::runs

#endif // SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_SERVICE_HPP
