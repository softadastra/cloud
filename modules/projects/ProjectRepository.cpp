/*
 * ProjectRepository.cpp
 */

#include "modules/projects/ProjectRepository.hpp"

#include <algorithm>
#include <stdexcept>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::modules::projects
{
  ProjectRepository::ProjectRepository(
      std::shared_ptr<vix::db::Database> database)
      : database_(std::move(database))
  {
    if (database_ == nullptr)
    {
      throw std::invalid_argument(
          "ProjectRepository requires a valid database instance");
    }
  }

  bool ProjectRepository::create(
      const std::string &public_id,
      std::int64_t user_id,
      const std::string &name,
      const std::string &slug,
      const std::string &description,
      const std::string &base_url,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "INSERT INTO projects ("
          "public_id, user_id, name, slug, description, base_url, created_at, updated_at"
          ") VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
          public_id,
          user_id,
          name,
          slug,
          description,
          base_url,
          now_ms,
          now_ms);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to create project",
          "slug", slug,
          "error", e.what());

      return false;
    }
  }

  std::vector<Project> ProjectRepository::all(
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "id, public_id, COALESCE(user_id, 0), name, slug, "
            "COALESCE(description, ''), COALESCE(base_url, ''), "
            "created_at, updated_at "
            "FROM projects "
            "ORDER BY id DESC "
            "LIMIT ? OFFSET ?",
            safe_limit,
            safe_offset);

    std::vector<Project> projects;

    while (rows->next())
    {
      projects.push_back(
          project_from_current_row(rows->row()));
    }

    return projects;
  }

  std::optional<Project> ProjectRepository::find_by_id(
      std::int64_t id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "id, public_id, COALESCE(user_id, 0), name, slug, "
            "COALESCE(description, ''), COALESCE(base_url, ''), "
            "created_at, updated_at "
            "FROM projects "
            "WHERE id = ? "
            "LIMIT 1",
            id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return project_from_current_row(rows->row());
  }

  std::optional<Project> ProjectRepository::find_by_public_id(
      const std::string &public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "id, public_id, COALESCE(user_id, 0), name, slug, "
            "COALESCE(description, ''), COALESCE(base_url, ''), "
            "created_at, updated_at "
            "FROM projects "
            "WHERE public_id = ? "
            "LIMIT 1",
            public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return project_from_current_row(rows->row());
  }

  std::optional<Project> ProjectRepository::find_by_slug(
      const std::string &slug) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "id, public_id, COALESCE(user_id, 0), name, slug, "
            "COALESCE(description, ''), COALESCE(base_url, ''), "
            "created_at, updated_at "
            "FROM projects "
            "WHERE slug = ? "
            "LIMIT 1",
            slug);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return project_from_current_row(rows->row());
  }

  bool ProjectRepository::slug_exists(
      const std::string &slug) const
  {
    auto rows =
        database_->query(
            "SELECT id FROM projects WHERE slug = ? LIMIT 1",
            slug);

    return rows->next();
  }

  bool ProjectRepository::update(
      const std::string &public_id,
      const std::string &name,
      const std::string &description,
      const std::string &base_url,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "UPDATE projects "
          "SET name = ?, description = ?, base_url = ?, updated_at = ? "
          "WHERE public_id = ?",
          name,
          description,
          base_url,
          now_ms,
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to update project",
          "project_id", public_id,
          "error", e.what());

      return false;
    }
  }

  bool ProjectRepository::remove(
      const std::string &public_id)
  {
    try
    {
      database_->exec(
          "DELETE FROM projects WHERE public_id = ?",
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to delete project",
          "project_id", public_id,
          "error", e.what());

      return false;
    }
  }

  std::int64_t ProjectRepository::count() const
  {
    auto rows =
        database_->query("SELECT COUNT(*) FROM projects");

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  Project ProjectRepository::project_from_current_row(
      const vix::db::Row &row)
  {
    Project project;

    project.id = row.getInt64(0);
    project.public_id = row.getString(1);
    project.user_id = row.getInt64(2);
    project.name = row.getString(3);
    project.slug = row.getString(4);
    project.description = row.getString(5);
    project.base_url = row.getString(6);
    project.created_at = row.getInt64(7);
    project.updated_at = row.getInt64(8);

    return project;
  }

  int ProjectRepository::normalize_limit(int limit) noexcept
  {
    return std::clamp(limit, 1, 100);
  }

  int ProjectRepository::normalize_offset(int offset) noexcept
  {
    return std::max(offset, 0);
  }

} // namespace softadastra::cloud::modules::projects
