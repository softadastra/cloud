/*
 * RunRepository.cpp
 */

#include "modules/runs/RunRepository.hpp"

#include <algorithm>
#include <stdexcept>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::modules::runs
{
  RunRepository::RunRepository(
      std::shared_ptr<vix::db::Database> database)
      : database_(std::move(database))
  {
    if (database_ == nullptr)
    {
      throw std::invalid_argument(
          "RunRepository requires a valid database instance");
    }
  }

  bool RunRepository::create(
      const std::string &public_id,
      std::int64_t project_id,
      std::int64_t agent_id,
      const std::string &status,
      const std::string &target_url,
      const std::string &method,
      const std::string &scenario,
      std::int64_t started_at,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "INSERT INTO runs ("
          "public_id, project_id, agent_id, status, target_url, method, "
          "scenario, started_at, finished_at, created_at, updated_at"
          ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          public_id,
          project_id,
          agent_id,
          status,
          target_url,
          method,
          scenario,
          started_at,
          0,
          now_ms,
          now_ms);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to create run",
          "project_id", project_id,
          "agent_id", agent_id,
          "target_url", target_url,
          "error", e.what());

      return false;
    }
  }

  std::vector<Run> RunRepository::all(
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "r.id, r.public_id, "
            "r.project_id, p.public_id, "
            "COALESCE(r.agent_id, 0), COALESCE(a.public_id, ''), "
            "r.status, r.target_url, r.method, r.scenario, "
            "r.started_at, COALESCE(r.finished_at, 0), "
            "r.created_at, r.updated_at "
            "FROM runs r "
            "JOIN projects p ON p.id = r.project_id "
            "LEFT JOIN agents a ON a.id = r.agent_id "
            "ORDER BY r.id DESC "
            "LIMIT ? OFFSET ?",
            safe_limit,
            safe_offset);

    std::vector<Run> runs;

    while (rows->next())
    {
      runs.push_back(
          run_from_current_row(rows->row()));
    }

    return runs;
  }

  std::vector<Run> RunRepository::all_for_project(
      const std::string &project_public_id,
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "r.id, r.public_id, "
            "r.project_id, p.public_id, "
            "COALESCE(r.agent_id, 0), COALESCE(a.public_id, ''), "
            "r.status, r.target_url, r.method, r.scenario, "
            "r.started_at, COALESCE(r.finished_at, 0), "
            "r.created_at, r.updated_at "
            "FROM runs r "
            "JOIN projects p ON p.id = r.project_id "
            "LEFT JOIN agents a ON a.id = r.agent_id "
            "WHERE p.public_id = ? "
            "ORDER BY r.id DESC "
            "LIMIT ? OFFSET ?",
            project_public_id,
            safe_limit,
            safe_offset);

    std::vector<Run> runs;

    while (rows->next())
    {
      runs.push_back(
          run_from_current_row(rows->row()));
    }

    return runs;
  }

  std::vector<Run> RunRepository::all_for_agent(
      const std::string &agent_public_id,
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "r.id, r.public_id, "
            "r.project_id, p.public_id, "
            "COALESCE(r.agent_id, 0), COALESCE(a.public_id, ''), "
            "r.status, r.target_url, r.method, r.scenario, "
            "r.started_at, COALESCE(r.finished_at, 0), "
            "r.created_at, r.updated_at "
            "FROM runs r "
            "JOIN projects p ON p.id = r.project_id "
            "LEFT JOIN agents a ON a.id = r.agent_id "
            "WHERE a.public_id = ? "
            "ORDER BY r.id DESC "
            "LIMIT ? OFFSET ?",
            agent_public_id,
            safe_limit,
            safe_offset);

    std::vector<Run> runs;

    while (rows->next())
    {
      runs.push_back(
          run_from_current_row(rows->row()));
    }

    return runs;
  }

  std::optional<Run> RunRepository::find_by_id(
      std::int64_t id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "r.id, r.public_id, "
            "r.project_id, p.public_id, "
            "COALESCE(r.agent_id, 0), COALESCE(a.public_id, ''), "
            "r.status, r.target_url, r.method, r.scenario, "
            "r.started_at, COALESCE(r.finished_at, 0), "
            "r.created_at, r.updated_at "
            "FROM runs r "
            "JOIN projects p ON p.id = r.project_id "
            "LEFT JOIN agents a ON a.id = r.agent_id "
            "WHERE r.id = ? "
            "LIMIT 1",
            id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return run_from_current_row(rows->row());
  }

  std::optional<Run> RunRepository::find_by_public_id(
      const std::string &public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "r.id, r.public_id, "
            "r.project_id, p.public_id, "
            "COALESCE(r.agent_id, 0), COALESCE(a.public_id, ''), "
            "r.status, r.target_url, r.method, r.scenario, "
            "r.started_at, COALESCE(r.finished_at, 0), "
            "r.created_at, r.updated_at "
            "FROM runs r "
            "JOIN projects p ON p.id = r.project_id "
            "LEFT JOIN agents a ON a.id = r.agent_id "
            "WHERE r.public_id = ? "
            "LIMIT 1",
            public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return run_from_current_row(rows->row());
  }

  bool RunRepository::update_status(
      const std::string &public_id,
      const std::string &status,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "UPDATE runs "
          "SET status = ?, updated_at = ? "
          "WHERE public_id = ?",
          status,
          now_ms,
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to update run status",
          "run_id", public_id,
          "status", status,
          "error", e.what());

      return false;
    }
  }

  bool RunRepository::finish(
      const std::string &public_id,
      const std::string &status,
      std::int64_t finished_at,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "UPDATE runs "
          "SET status = ?, finished_at = ?, updated_at = ? "
          "WHERE public_id = ?",
          status,
          finished_at,
          now_ms,
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to finish run",
          "run_id", public_id,
          "status", status,
          "error", e.what());

      return false;
    }
  }

  bool RunRepository::remove(
      const std::string &public_id)
  {
    try
    {
      database_->exec(
          "DELETE FROM runs WHERE public_id = ?",
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to delete run",
          "run_id", public_id,
          "error", e.what());

      return false;
    }
  }

  std::int64_t RunRepository::count() const
  {
    auto rows =
        database_->query("SELECT COUNT(*) FROM runs");

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t RunRepository::count_for_project(
      const std::string &project_public_id) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) "
            "FROM runs r "
            "JOIN projects p ON p.id = r.project_id "
            "WHERE p.public_id = ?",
            project_public_id);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t RunRepository::count_for_agent(
      const std::string &agent_public_id) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) "
            "FROM runs r "
            "JOIN agents a ON a.id = r.agent_id "
            "WHERE a.public_id = ?",
            agent_public_id);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t RunRepository::count_by_status(
      const std::string &status) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) FROM runs WHERE status = ?",
            status);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  Run RunRepository::run_from_current_row(
      const vix::db::ResultRow &row)
  {
    Run run;

    run.id = row.getInt64(0);
    run.public_id = row.getString(1);
    run.project_id = row.getInt64(2);
    run.project_public_id = row.getString(3);
    run.agent_id = row.getInt64(4);
    run.agent_public_id = row.getString(5);
    run.status = row.getString(6);
    run.target_url = row.getString(7);
    run.method = row.getString(8);
    run.scenario = row.getString(9);
    run.started_at = row.getInt64(10);
    run.finished_at = row.getInt64(11);
    run.created_at = row.getInt64(12);
    run.updated_at = row.getInt64(13);

    return run;
  }

  int RunRepository::normalize_limit(int limit) noexcept
  {
    return std::clamp(limit, 1, 100);
  }

  int RunRepository::normalize_offset(int offset) noexcept
  {
    return std::max(offset, 0);
  }

} // namespace softadastra::cloud::modules::runs
