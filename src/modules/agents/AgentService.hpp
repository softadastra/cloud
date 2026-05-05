/**
 *
 *  @file AgentService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_SERVICE_HPP

#include <string>
#include <vector>

#include "app/AppState.hpp"
#include "modules/agents/Agent.hpp"
#include "modules/agents/AgentRepository.hpp"
#include "modules/projects/ProjectRepository.hpp"

namespace softadastra::cloud::modules::agents
{
  /**
   * @brief Result returned by agent operations.
   */
  struct AgentResult
  {
    bool success{false};
    int status{400};

    std::string error{"agent_error"};
    std::string message{};

    Agent agent{};

    /**
     * @brief Plain API key generated during creation or rotation.
     *
     * This value must only be returned once to the caller.
     * The database stores only the hash.
     */
    std::string api_key{};
  };

  /**
   * @brief List result returned by agent listing.
   */
  struct AgentListResult
  {
    bool success{false};
    int status{400};

    std::string error{"agent_error"};
    std::string message{};

    std::vector<Agent> agents{};
    std::int64_t total{0};
    int limit{20};
    int offset{0};
  };

  /**
   * @brief Agent business service.
   *
   * AgentService owns the business rules for agents:
   * - validating agent input
   * - checking project existence
   * - generating agent ids
   * - generating API keys
   * - hashing API keys before storage
   * - rotating keys
   *
   * It does not read Vix Request and does not write Vix Response.
   */
  class AgentService final
  {
  public:
    /**
     * @brief Creates an agent service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit AgentService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Creates a new agent for a project.
     *
     * @param project_public_id Public project id.
     * @param name Agent display name.
     * @return Agent result with one-time plain API key.
     */
    [[nodiscard]] AgentResult create_agent(
        const std::string &project_public_id,
        const std::string &name);

    /**
     * @brief Lists all agents.
     *
     * @param limit Max number of agents.
     * @param offset Pagination offset.
     * @return Agent list result.
     */
    [[nodiscard]] AgentListResult list_agents(
        int limit,
        int offset);

    /**
     * @brief Lists agents for a project.
     *
     * @param project_public_id Public project id.
     * @param limit Max number of agents.
     * @param offset Pagination offset.
     * @return Agent list result.
     */
    [[nodiscard]] AgentListResult list_agents_for_project(
        const std::string &project_public_id,
        int limit,
        int offset);

    /**
     * @brief Gets one agent by public id.
     *
     * @param public_id Public agent id.
     * @return Agent result.
     */
    [[nodiscard]] AgentResult get_agent(
        const std::string &public_id);

    /**
     * @brief Rotates one agent API key.
     *
     * @param public_id Public agent id.
     * @return Agent result with one-time plain API key.
     */
    [[nodiscard]] AgentResult rotate_api_key(
        const std::string &public_id);

    /**
     * @brief Deletes one agent.
     *
     * @param public_id Public agent id.
     * @return Agent result.
     */
    [[nodiscard]] AgentResult delete_agent(
        const std::string &public_id);

    /**
     * @brief Authenticates an agent using a plain API key.
     *
     * @param api_key Plain API key from request header.
     * @return Agent result.
     */
    [[nodiscard]] AgentResult authenticate_api_key(
        const std::string &api_key);

  private:
    const softadastra::cloud::app::AppState &state_;
    AgentRepository repository_;
    softadastra::cloud::modules::projects::ProjectRepository project_repository_;

    [[nodiscard]] static bool is_valid_name(
        const std::string &name) noexcept;

    [[nodiscard]] static std::string generate_api_key();

    [[nodiscard]] static std::string hash_api_key(
        const softadastra::cloud::app::AppState &state,
        const std::string &api_key);

    [[nodiscard]] static AgentResult validation_error(
        std::string message);

    [[nodiscard]] static AgentResult unauthorized(
        std::string message);

    [[nodiscard]] static AgentResult not_found(
        std::string message);

    [[nodiscard]] static AgentResult internal_error(
        std::string message);

    [[nodiscard]] static AgentListResult list_error(
        int status,
        std::string error,
        std::string message);
  };

} // namespace softadastra::cloud::modules::agents

#endif // SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_SERVICE_HPP
