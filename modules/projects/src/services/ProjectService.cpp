/**
 *
 *  @file ProjectService.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Softadastra Cloud
 */
#include <projects/services/ProjectService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::projects::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_project_id()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "project_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    std::string normalize_slug(
        const std::string &value)
    {
      std::string slug;
      bool previous_dash = false;

      for (const char ch : value)
      {
        const auto c = static_cast<unsigned char>(ch);

        if (std::isalnum(c))
        {
          slug.push_back(
              static_cast<char>(std::tolower(c)));
          previous_dash = false;
          continue;
        }

        if (!previous_dash && !slug.empty())
        {
          slug.push_back('-');
          previous_dash = true;
        }
      }

      while (!slug.empty() && slug.back() == '-')
      {
        slug.pop_back();
      }

      return slug;
    }

    bool valid_name(
        const std::string &name)
    {
      return name.size() >= 2 && name.size() <= 100;
    }

    bool valid_slug(
        const std::string &slug)
    {
      if (slug.size() < 2 || slug.size() > 100)
      {
        return false;
      }

      for (const char ch : slug)
      {
        const auto c = static_cast<unsigned char>(ch);

        if (std::isalnum(c) || ch == '-')
        {
          continue;
        }

        return false;
      }

      return slug.front() != '-' && slug.back() != '-';
    }

    std::string project_slug_key(
        const std::string &workspace_id,
        const std::string &slug)
    {
      return workspace_id + "::" + slug;
    }

    std::string default_branch_or_main(
        const std::string &branch)
    {
      if (branch.empty())
      {
        return "main";
      }

      return branch;
    }
  } // namespace

  class ProjectService::Impl
  {
  public:
    Impl()
    {
      const char *use_db = std::getenv("SOFTADASTRA_CLOUD_USE_DB");

      if (use_db != nullptr && std::string(use_db) == "1")
      {
        vix::config::Config cfg{".env"};
        db = std::make_unique<vix::db::Database>(cfg);
      }
    }

    bool persistent() const
    {
      return db != nullptr;
    }

    dto::ProjectResponse row_to_project(
        const vix::db::ResultRow &row) const
    {
      dto::ProjectResponse project;
      project.id = row.getString(0);
      project.workspace_id = row.getString(1);
      project.owner_user_id = row.getString(2);
      project.name = row.getString(3);
      project.slug = row.getString(4);
      project.description = row.getString(5);
      project.repository_url = row.getString(6);
      project.default_branch = row.getString(7);
      project.active = row.getInt64(8) != 0;
      project.created_at = row.getInt64(9);
      project.updated_at = row.getInt64(10);
      return project;
    }

    bool workspace_exists(
        const std::string &workspace_id) const
    {
      auto rows = db->query(
          "SELECT id FROM workspaces WHERE id = ? LIMIT 1",
          workspace_id);

      return rows->next();
    }

    bool project_slug_exists(
        const std::string &workspace_id,
        const std::string &slug,
        const std::string &except_id = "") const
    {
      auto rows = db->query(
          "SELECT id FROM projects WHERE workspace_id = ? AND slug = ? LIMIT 1",
          workspace_id,
          slug);

      if (!rows->next())
      {
        return false;
      }

      return except_id.empty() || rows->row().getString(0) != except_id;
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::ProjectResponse> projects_by_id;
    std::unordered_map<std::string, std::string> project_id_by_workspace_slug;
  };

  ProjectService::ProjectService()
      : impl_(std::make_unique<Impl>())
  {
  }

  ProjectService::~ProjectService() = default;

  ProjectService::ProjectService(ProjectService &&) noexcept = default;

  ProjectService &ProjectService::operator=(ProjectService &&) noexcept = default;

  ProjectResult<dto::ProjectResponse> ProjectService::create_project(
      const dto::CreateProjectRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::MissingWorkspace,
                                                           "Workspace is required."});
    }

    if (request.owner_user_id.empty())
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::MissingOwner,
                                                           "Project owner is required."});
    }

    if (!valid_name(request.name))
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::InvalidName,
                                                           "Project name must contain between 2 and 100 characters."});
    }

    const auto slug = request.slug.empty()
                          ? normalize_slug(request.name)
                          : normalize_slug(request.slug);

    if (!valid_slug(slug))
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::InvalidSlug,
                                                           "Project slug must contain only letters, numbers and dashes."});
    }

    if (impl_->persistent())
    {
      try
      {
        if (!impl_->workspace_exists(request.workspace_id))
        {
          return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::MissingWorkspace,
                                                               "Workspace not found."});
        }

        if (impl_->project_slug_exists(request.workspace_id, slug))
        {
          return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectAlreadyExists,
                                                               "Project slug is already used in this workspace."});
        }

        const auto timestamp = now_timestamp();

        dto::ProjectResponse project;
        project.id = make_project_id();
        project.workspace_id = request.workspace_id;
        project.owner_user_id = request.owner_user_id;
        project.name = request.name;
        project.slug = slug;
        project.description = request.description;
        project.repository_url = request.repository_url;
        project.default_branch = default_branch_or_main(request.default_branch);
        project.active = true;
        project.created_at = timestamp;
        project.updated_at = timestamp;

        impl_->db->exec(
            "INSERT INTO projects "
            "(id, workspace_id, owner_user_id, name, slug, description, repository_url, default_branch, active, created_at, updated_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            project.id,
            project.workspace_id,
            project.owner_user_id,
            project.name,
            project.slug,
            project.description,
            project.repository_url,
            project.default_branch,
            static_cast<std::int64_t>(project.active ? 1 : 0),
            project.created_at,
            project.updated_at);

        return ProjectResult<dto::ProjectResponse>::success(project);
      }
      catch (const std::exception &error)
      {
        (void)error;
        return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectAlreadyExists,
                                                             "Project slug is already used in this workspace."});
      }
    }

    const auto key = project_slug_key(request.workspace_id, slug);

    if (impl_->project_id_by_workspace_slug.find(key) !=
        impl_->project_id_by_workspace_slug.end())
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectAlreadyExists,
                                                           "Project slug is already used in this workspace."});
    }

    const auto timestamp = now_timestamp();

    dto::ProjectResponse project;
    project.id = make_project_id();
    project.workspace_id = request.workspace_id;
    project.owner_user_id = request.owner_user_id;
    project.name = request.name;
    project.slug = slug;
    project.description = request.description;
    project.repository_url = request.repository_url;
    project.default_branch = default_branch_or_main(request.default_branch);
    project.active = true;
    project.created_at = timestamp;
    project.updated_at = timestamp;

    impl_->project_id_by_workspace_slug[key] = project.id;
    impl_->projects_by_id[project.id] = project;

    return ProjectResult<dto::ProjectResponse>::success(project);
  }

  ProjectResult<dto::ProjectResponse> ProjectService::update_project(
      const dto::UpdateProjectRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::MissingWorkspace,
                                                           "Workspace is required."});
    }

    if (impl_->persistent())
    {
      auto current = find_project({request.id, request.workspace_id});

      if (current.failed())
      {
        return current;
      }

      if (!valid_name(request.name))
      {
        return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::InvalidName,
                                                             "Project name must contain between 2 and 100 characters."});
      }

      const auto slug = request.slug.empty()
                            ? normalize_slug(request.name)
                            : normalize_slug(request.slug);

      if (!valid_slug(slug))
      {
        return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::InvalidSlug,
                                                             "Project slug must contain only letters, numbers and dashes."});
      }

      if (impl_->project_slug_exists(request.workspace_id, slug, request.id))
      {
        return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectAlreadyExists,
                                                             "Project slug is already used in this workspace."});
      }

      auto project = current.value();
      project.name = request.name;
      project.slug = slug;
      project.description = request.description;
      project.repository_url = request.repository_url;
      project.default_branch = default_branch_or_main(request.default_branch);
      project.updated_at = now_timestamp();

      impl_->db->exec(
          "UPDATE projects "
          "SET name = ?, slug = ?, description = ?, repository_url = ?, default_branch = ?, updated_at = ? "
          "WHERE id = ? AND workspace_id = ?",
          project.name,
          project.slug,
          project.description,
          project.repository_url,
          project.default_branch,
          project.updated_at,
          project.id,
          project.workspace_id);

      return ProjectResult<dto::ProjectResponse>::success(project);
    }

    auto item = impl_->projects_by_id.find(request.id);

    if (item == impl_->projects_by_id.end() ||
        item->second.workspace_id != request.workspace_id)
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectNotFound,
                                                           "Project not found."});
    }

    if (!valid_name(request.name))
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::InvalidName,
                                                           "Project name must contain between 2 and 100 characters."});
    }

    const auto slug = request.slug.empty()
                          ? normalize_slug(request.name)
                          : normalize_slug(request.slug);

    if (!valid_slug(slug))
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::InvalidSlug,
                                                           "Project slug must contain only letters, numbers and dashes."});
    }

    const auto new_key = project_slug_key(request.workspace_id, slug);
    auto existing = impl_->project_id_by_workspace_slug.find(new_key);

    if (existing != impl_->project_id_by_workspace_slug.end() &&
        existing->second != request.id)
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectAlreadyExists,
                                                           "Project slug is already used in this workspace."});
    }

    const auto old_key = project_slug_key(
        item->second.workspace_id,
        item->second.slug);

    impl_->project_id_by_workspace_slug.erase(old_key);

    item->second.name = request.name;
    item->second.slug = slug;
    item->second.description = request.description;
    item->second.repository_url = request.repository_url;
    item->second.default_branch = default_branch_or_main(request.default_branch);
    item->second.updated_at = now_timestamp();

    impl_->project_id_by_workspace_slug[new_key] = item->second.id;

    return ProjectResult<dto::ProjectResponse>::success(item->second);
  }

  ProjectResult<dto::ProjectResponse> ProjectService::find_project(
      const dto::ProjectLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::MissingWorkspace,
                                                           "Workspace is required."});
    }

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, owner_user_id, name, slug, description, repository_url, default_branch, active, created_at, updated_at "
          "FROM projects WHERE id = ? AND workspace_id = ? LIMIT 1",
          request.id,
          request.workspace_id);

      if (!rows->next())
      {
        return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectNotFound,
                                                             "Project not found."});
      }

      return ProjectResult<dto::ProjectResponse>::success(
          impl_->row_to_project(rows->row()));
    }

    auto item = impl_->projects_by_id.find(request.id);

    if (item == impl_->projects_by_id.end() ||
        item->second.workspace_id != request.workspace_id)
    {
      return ProjectResult<dto::ProjectResponse>::failure({support::ProjectErrorCode::ProjectNotFound,
                                                           "Project not found."});
    }

    return ProjectResult<dto::ProjectResponse>::success(item->second);
  }

  ProjectResult<std::vector<dto::ProjectResponse>> ProjectService::list_projects(
      const dto::ListProjectsRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return ProjectResult<std::vector<dto::ProjectResponse>>::failure({support::ProjectErrorCode::MissingWorkspace,
                                                                        "Workspace is required."});
    }

    std::vector<dto::ProjectResponse> projects;

    if (impl_->persistent())
    {
      if (request.access_scope == "selected_projects")
      {
        auto rows = impl_->db->query(
            "SELECT id, workspace_id, owner_user_id, name, slug, description, repository_url, default_branch, active, created_at, updated_at "
            "FROM projects WHERE workspace_id = ? AND instr(?, char(34) || id || char(34)) > 0 ORDER BY created_at",
            request.workspace_id,
            request.project_ids_json);

        while (rows->next())
        {
          projects.push_back(impl_->row_to_project(rows->row()));
        }
      }
      else
      {
        auto rows = impl_->db->query(
            "SELECT id, workspace_id, owner_user_id, name, slug, description, repository_url, default_branch, active, created_at, updated_at "
            "FROM projects WHERE workspace_id = ? ORDER BY created_at",
            request.workspace_id);

        while (rows->next())
        {
          projects.push_back(impl_->row_to_project(rows->row()));
        }
      }

      return ProjectResult<std::vector<dto::ProjectResponse>>::success(projects);
    }

    for (const auto &entry : impl_->projects_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id)
      {
        projects.push_back(entry.second);
      }
    }

    std::sort(
        projects.begin(),
        projects.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return ProjectResult<std::vector<dto::ProjectResponse>>::success(projects);
  }
} // namespace cloud::projects::services
