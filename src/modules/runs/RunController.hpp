/**
 *
 *  @file RunController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::runs
{
  /**
   * @brief HTTP controller for reliability run endpoints.
   *
   * RunController handles only HTTP concerns:
   * - read path/query parameters
   * - read JSON request body
   * - call RunService
   * - return standardized JSON responses
   *
   * Business rules belong in RunService.
   * SQL belongs in RunRepository.
   */
  class RunController final
  {
  public:
    /**
     * @brief Creates a new reliability run.
     *
     * Expected JSON body:
     * {
     *   "project_id": "prj_xxx",
     *   "agent_id": "agt_xxx",
     *   "target_url": "https://api.example.com/orders",
     *   "method": "POST",
     *   "scenario": "unstable_network"
     * }
     */
    static void create(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Lists runs.
     *
     * Query params:
     * - limit
     * - offset
     * - project_id optional
     * - agent_id optional
     */
    static void list(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Gets one run by public id.
     *
     * Path:
     * /api/runs/{id}
     */
    static void get(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Updates one run status.
     *
     * Expected JSON body:
     * {
     *   "status": "running"
     * }
     */
    static void update_status(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Finishes one run.
     *
     * Expected JSON body:
     * {
     *   "status": "finished"
     * }
     */
    static void finish(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Deletes one run by public id.
     *
     * Path:
     * /api/runs/{id}
     */
    static void remove(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    RunController() = delete;
  };

} // namespace softadastra::cloud::modules::runs

#endif // SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_CONTROLLER_HPP
