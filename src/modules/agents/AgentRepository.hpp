/**
 *
 *  @file AgentRepository.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_REPOSITORY_HPP
#define SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/db/db.hpp>

#include "modules/agents/Agent.hpp"

namespace softadastra::cloud::modules::agents
{
  /**
   * @brief Repository for agent persistence.
   *
   * AgentRepository owns all SQL access for Softadastra Cloud agents.
   *
   * It does not contain HTTP logic or business rules. Those belong to
   * AgentController and AgentService.
   */
  class AgentRepository final
  {
  public:
    /**
     * @brief Creates an agent repository.
     *
     * @param database Shared Vix database instance.
     */
    explicit AgentRepository(
        std::shared_ptr<vix::db::Database> database);

    /**
     * @brief Creates an agent for a project.
     *
     * @param public_id Public agent id.
     * @param project_id Internal project id.
     * @param name Agent display name.
     * @param api_key_hash Stored API key hash.
     * @param now_ms Creation/update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool create(
        const std::string &public_id,
        std::int64_t project_id,
        const std::string &name,
        const std::string &api_key_hash,
        std::int64_t now_ms);

    /**
     * @brief Returns all agents.
     *
     * @param limit Max number of agents.
     * @param offset Offset for pagination.
     * @return Agent list.
     */
    [[nodiscard]] std::vector<Agent> all(
        int limit,
        int offset) const;

    /**
     * @brief Returns all agents attached to a project.
     *
     * @param project_public_id Public project id.
     * @param limit Max number of agents.
     * @param offset Offset for pagination.
     * @return Agent list.
     */
    [[nodiscard]] std::vector<Agent> all_for_project(
        const std::string &project_public_id,
        int limit,
        int offset) const;

    /**
     * @brief Finds an agent by internal id.
     *
     * @param id Internal database id.
     * @return Agent if found.
     */
    [[nodiscard]] std::optional<Agent> find_by_id(
        std::int64_t id) const;

    /**
     * @brief Finds an agent by public id.
     *
     * @param public_id Public agent id.
     * @return Agent if found.
     */
    [[nodiscard]] std::optional<Agent> find_by_public_id(
        const std::string &public_id) const;

    /**
     * @brief Finds an agent by API key hash.
     *
     * @param api_key_hash Stored API key hash.
     * @return Agent if found.
     */
    [[nodiscard]] std::optional<Agent> find_by_api_key_hash(
        const std::string &api_key_hash) const;

    /**
     * @brief Updates agent last_seen_at.
     *
     * @param public_id Public agent id.
     * @param now_ms Last seen timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool touch_last_seen(
        const std::string &public_id,
        std::int64_t now_ms);

    /**
     * @brief Rotates an agent API key hash.
     *
     * @param public_id Public agent id.
     * @param api_key_hash New API key hash.
     * @param now_ms Update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool rotate_api_key(
        const std::string &public_id,
        const std::string &api_key_hash,
        std::int64_t now_ms);

    /**
     * @brief Deletes an agent by public id.
     *
     * @param public_id Public agent id.
     * @return true on success.
     */
    [[nodiscard]] bool remove(
        const std::string &public_id);

    /**
     * @brief Counts all agents.
     *
     * @return Agent count.
     */
    [[nodiscard]] std::int64_t count() const;

    /**
     * @brief Counts all agents for one project.
     *
     * @param project_public_id Public project id.
     * @return Agent count.
     */
    [[nodiscard]] std::int64_t count_for_project(
        const std::string &project_public_id) const;

  private:
    std::shared_ptr<vix::db::Database> database_;

    [[nodiscard]] static Agent agent_from_current_row(
        const vix::db::ResultRow &row);

    [[nodiscard]] static int normalize_limit(int limit) noexcept;
    [[nodiscard]] static int normalize_offset(int offset) noexcept;
  };

} // namespace softadastra::cloud::modules::agents

#endif // SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_REPOSITORY_HPP
