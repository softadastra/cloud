/**
 *
 *  @file AgentController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::agents
{
  /**
   * @brief HTTP controller for agent endpoints.
   *
   * AgentController handles only HTTP concerns:
   * - read path/query parameters
   * - read JSON request body
   * - call AgentService
   * - return standardized JSON responses
   *
   * Business rules belong in AgentService.
   * SQL belongs in AgentRepository.
   */
  class AgentController final
  {
  public:
    /**
     * @brief Creates a new agent for a project.
     *
     * Expected JSON body:
     * {
     *   "project_id": "prj_xxx",
     *   "name": "Local CLI Agent"
     * }
     */
    static void create(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Lists agents.
     *
     * Query params:
     * - limit
     * - offset
     * - project_id optional
     */
    static void list(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Gets one agent by public id.
     *
     * Path:
     * /api/agents/{id}
     */
    static void get(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Rotates one agent API key.
     *
     * Path:
     * /api/agents/{id}/rotate-key
     */
    static void rotate_key(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Deletes one agent by public id.
     *
     * Path:
     * /api/agents/{id}
     */
    static void remove(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Verifies the current agent API key.
     *
     * This endpoint is useful for CLI agents to check that their API key works.
     *
     * Header:
     * x-agent-api-key: ...
     */
    static void verify(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    AgentController() = delete;
  };

} // namespace softadastra::cloud::modules::agents

#endif // SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_CONTROLLER_HPP
