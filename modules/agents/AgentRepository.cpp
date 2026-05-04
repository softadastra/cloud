/*
 * AgentRepository.cpp
 */

#include "modules/agents/AgentRepository.hpp"

#include <algorithm>
#include <stdexcept>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::modules::agents
{
  AgentRepository::AgentRepository(
      std::shared_ptr<vix::db::Database> database)
      : database_(std::move(database))
  {
    if (database_ == nullptr)
    {
      throw std::invalid_argument(
          "AgentRepository requires a valid database instance");
    }
  }

  bool AgentRepository::create(
      const std::string &public_id,
      std::int64_t project_id,
      const std::string &name,
      const std::string &api_key_hash,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "INSERT INTO agents ("
          "public_id, project_id, name, api_key_hash, last_seen_at, created_at, updated_at"
          ") VALUES (?, ?, ?, ?, ?, ?, ?)",
          public_id,
          project_id,
          name,
          api_key_hash,
          0,
          now_ms,
          now_ms);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to create agent",
          "project_id", project_id,
          "name", name,
          "error", e.what());

      return false;
    }
  }

  std::vector<Agent> AgentRepository::all(
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "a.id, a.public_id, a.project_id, p.public_id, "
            "a.name, a.api_key_hash, "
            "COALESCE(a.last_seen_at, 0), "
            "a.created_at, a.updated_at "
            "FROM agents a "
            "JOIN projects p ON p.id = a.project_id "
            "ORDER BY a.id DESC "
            "LIMIT ? OFFSET ?",
            safe_limit,
            safe_offset);

    std::vector<Agent> agents;

    while (rows->next())
    {
      agents.push_back(
          agent_from_current_row(rows->row()));
    }

    return agents;
  }

  std::vector<Agent> AgentRepository::all_for_project(
      const std::string &project_public_id,
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "a.id, a.public_id, a.project_id, p.public_id, "
            "a.name, a.api_key_hash, "
            "COALESCE(a.last_seen_at, 0), "
            "a.created_at, a.updated_at "
            "FROM agents a "
            "JOIN projects p ON p.id = a.project_id "
            "WHERE p.public_id = ? "
            "ORDER BY a.id DESC "
            "LIMIT ? OFFSET ?",
            project_public_id,
            safe_limit,
            safe_offset);

    std::vector<Agent> agents;

    while (rows->next())
    {
      agents.push_back(
          agent_from_current_row(rows->row()));
    }

    return agents;
  }

  std::optional<Agent> AgentRepository::find_by_id(
      std::int64_t id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "a.id, a.public_id, a.project_id, p.public_id, "
            "a.name, a.api_key_hash, "
            "COALESCE(a.last_seen_at, 0), "
            "a.created_at, a.updated_at "
            "FROM agents a "
            "JOIN projects p ON p.id = a.project_id "
            "WHERE a.id = ? "
            "LIMIT 1",
            id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return agent_from_current_row(rows->row());
  }

  std::optional<Agent> AgentRepository::find_by_public_id(
      const std::string &public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "a.id, a.public_id, a.project_id, p.public_id, "
            "a.name, a.api_key_hash, "
            "COALESCE(a.last_seen_at, 0), "
            "a.created_at, a.updated_at "
            "FROM agents a "
            "JOIN projects p ON p.id = a.project_id "
            "WHERE a.public_id = ? "
            "LIMIT 1",
            public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return agent_from_current_row(rows->row());
  }

  std::optional<Agent> AgentRepository::find_by_api_key_hash(
      const std::string &api_key_hash) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "a.id, a.public_id, a.project_id, p.public_id, "
            "a.name, a.api_key_hash, "
            "COALESCE(a.last_seen_at, 0), "
            "a.created_at, a.updated_at "
            "FROM agents a "
            "JOIN projects p ON p.id = a.project_id "
            "WHERE a.api_key_hash = ? "
            "LIMIT 1",
            api_key_hash);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return agent_from_current_row(rows->row());
  }

  bool AgentRepository::touch_last_seen(
      const std::string &public_id,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "UPDATE agents "
          "SET last_seen_at = ?, updated_at = ? "
          "WHERE public_id = ?",
          now_ms,
          now_ms,
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to update agent last_seen_at",
          "agent_id", public_id,
          "error", e.what());

      return false;
    }
  }

  bool AgentRepository::rotate_api_key(
      const std::string &public_id,
      const std::string &api_key_hash,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "UPDATE agents "
          "SET api_key_hash = ?, updated_at = ? "
          "WHERE public_id = ?",
          api_key_hash,
          now_ms,
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to rotate agent api key",
          "agent_id", public_id,
          "error", e.what());

      return false;
    }
  }

  bool AgentRepository::remove(
      const std::string &public_id)
  {
    try
    {
      database_->exec(
          "DELETE FROM agents WHERE public_id = ?",
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to delete agent",
          "agent_id", public_id,
          "error", e.what());

      return false;
    }
  }

  std::int64_t AgentRepository::count() const
  {
    auto rows =
        database_->query("SELECT COUNT(*) FROM agents");

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t AgentRepository::count_for_project(
      const std::string &project_public_id) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) "
            "FROM agents a "
            "JOIN projects p ON p.id = a.project_id "
            "WHERE p.public_id = ?",
            project_public_id);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  Agent AgentRepository::agent_from_current_row(
      const vix::db::Row &row)
  {
    Agent agent;

    agent.id = row.getInt64(0);
    agent.public_id = row.getString(1);
    agent.project_id = row.getInt64(2);
    agent.project_public_id = row.getString(3);
    agent.name = row.getString(4);
    agent.api_key_hash = row.getString(5);
    agent.last_seen_at = row.getInt64(6);
    agent.created_at = row.getInt64(7);
    agent.updated_at = row.getInt64(8);

    return agent;
  }

  int AgentRepository::normalize_limit(int limit) noexcept
  {
    return std::clamp(limit, 1, 100);
  }

  int AgentRepository::normalize_offset(int offset) noexcept
  {
    return std::max(offset, 0);
  }

} // namespace softadastra::cloud::modules::agents
