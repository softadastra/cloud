/**
 *
 *  @file WorkspaceService.cpp
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
#include <workspaces/services/WorkspaceService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::workspaces::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_workspace_id()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "workspace_"
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
      return name.size() >= 2 && name.size() <= 80;
    }

    bool valid_slug(
        const std::string &slug)
    {
      if (slug.size() < 2 || slug.size() > 80)
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
  } // namespace

  class WorkspaceService::Impl
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

    dto::WorkspaceResponse row_to_workspace(
        const vix::db::ResultRow &row) const
    {
      dto::WorkspaceResponse workspace;
      workspace.id = row.getString(0);
      workspace.owner_user_id = row.getString(1);
      workspace.name = row.getString(2);
      workspace.slug = row.getString(3);
      workspace.active = row.getInt64(4) != 0;
      workspace.created_at = row.getInt64(5);
      workspace.updated_at = row.getInt64(6);
      return workspace;
    }

    dto::WorkspaceResponse row_to_workspace_with_role(
        const vix::db::ResultRow &row) const
    {
      auto workspace = row_to_workspace(row);
      workspace.current_user_role = row.getString(7);
      workspace.current_user_status = row.getString(8);
      workspace.access_scope = row.getString(9);
      workspace.project_ids_json = row.getString(10);
      workspace.current_user_is_owner = workspace.current_user_role == "owner";
      return workspace;
    }

    bool workspace_slug_exists(
        const std::string &slug,
        const std::string &except_id = "") const
    {
      auto rows = db->query(
          "SELECT id FROM workspaces WHERE slug = ? LIMIT 1",
          slug);

      if (!rows->next())
      {
        return false;
      }

      return except_id.empty() || rows->row().getString(0) != except_id;
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::WorkspaceResponse> workspaces_by_id;
    std::unordered_map<std::string, std::string> workspace_id_by_slug;
  };

  WorkspaceService::WorkspaceService()
      : impl_(std::make_unique<Impl>())
  {
  }

  WorkspaceService::~WorkspaceService() = default;

  WorkspaceService::WorkspaceService(WorkspaceService &&) noexcept = default;

  WorkspaceService &WorkspaceService::operator=(WorkspaceService &&) noexcept = default;

  WorkspaceResult<dto::WorkspaceResponse> WorkspaceService::create_workspace(
      const dto::CreateWorkspaceRequest &request)
  {
    if (!valid_name(request.name))
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::InvalidName,
                                                               "Workspace name must contain between 2 and 80 characters."});
    }

    if (request.owner_user_id.empty())
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::MissingOwner,
                                                               "Workspace owner is required."});
    }

    auto slug = request.slug.empty()
                    ? normalize_slug(request.name)
                    : normalize_slug(request.slug);

    if (!valid_slug(slug))
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::InvalidSlug,
                                                               "Workspace slug must contain only letters, numbers and dashes."});
    }

    if (impl_->persistent())
    {
      try
      {
        if (impl_->workspace_slug_exists(slug))
        {
          return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceAlreadyExists,
                                                                   "Workspace slug is already used."});
        }

        const auto timestamp = now_timestamp();

        dto::WorkspaceResponse workspace;
        workspace.id = make_workspace_id();
        workspace.name = request.name;
        workspace.slug = slug;
        workspace.owner_user_id = request.owner_user_id;
        workspace.active = true;
        workspace.created_at = timestamp;
        workspace.updated_at = timestamp;

        impl_->db->exec(
            "INSERT INTO workspaces "
            "(id, owner_user_id, name, slug, active, created_at, updated_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?)",
            workspace.id,
            workspace.owner_user_id,
            workspace.name,
            workspace.slug,
            static_cast<std::int64_t>(workspace.active ? 1 : 0),
            workspace.created_at,
            workspace.updated_at);

        return WorkspaceResult<dto::WorkspaceResponse>::success(workspace);
      }
      catch (const std::exception &error)
      {
        (void)error;
        return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceAlreadyExists,
                                                                 "Workspace slug is already used."});
      }
    }

    if (impl_->workspace_id_by_slug.find(slug) != impl_->workspace_id_by_slug.end())
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceAlreadyExists,
                                                               "Workspace slug is already used."});
    }

    const auto timestamp = now_timestamp();

    dto::WorkspaceResponse workspace;
    workspace.id = make_workspace_id();
    workspace.name = request.name;
    workspace.slug = slug;
    workspace.owner_user_id = request.owner_user_id;
    workspace.active = true;
    workspace.created_at = timestamp;
    workspace.updated_at = timestamp;

    impl_->workspace_id_by_slug[workspace.slug] = workspace.id;
    impl_->workspaces_by_id[workspace.id] = workspace;

    return WorkspaceResult<dto::WorkspaceResponse>::success(workspace);
  }

  WorkspaceResult<dto::WorkspaceResponse> WorkspaceService::update_workspace(
      const dto::UpdateWorkspaceRequest &request)
  {
    if (impl_->persistent())
    {
      auto current = find_workspace({request.id});

      if (current.failed())
      {
        return current;
      }

      if (!valid_name(request.name))
      {
        return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::InvalidName,
                                                                 "Workspace name must contain between 2 and 80 characters."});
      }

      const auto slug = request.slug.empty()
                            ? normalize_slug(request.name)
                            : normalize_slug(request.slug);

      if (!valid_slug(slug))
      {
        return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::InvalidSlug,
                                                                 "Workspace slug must contain only letters, numbers and dashes."});
      }

      if (impl_->workspace_slug_exists(slug, request.id))
      {
        return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceAlreadyExists,
                                                                 "Workspace slug is already used."});
      }

      auto workspace = current.value();
      workspace.name = request.name;
      workspace.slug = slug;
      workspace.updated_at = now_timestamp();

      impl_->db->exec(
          "UPDATE workspaces "
          "SET name = ?, slug = ?, updated_at = ? "
          "WHERE id = ?",
          workspace.name,
          workspace.slug,
          workspace.updated_at,
          workspace.id);

      return WorkspaceResult<dto::WorkspaceResponse>::success(workspace);
    }

    auto item = impl_->workspaces_by_id.find(request.id);

    if (item == impl_->workspaces_by_id.end())
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceNotFound,
                                                               "Workspace not found."});
    }

    if (!valid_name(request.name))
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::InvalidName,
                                                               "Workspace name must contain between 2 and 80 characters."});
    }

    const auto slug = request.slug.empty()
                          ? normalize_slug(request.name)
                          : normalize_slug(request.slug);

    if (!valid_slug(slug))
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::InvalidSlug,
                                                               "Workspace slug must contain only letters, numbers and dashes."});
    }

    auto existing_slug = impl_->workspace_id_by_slug.find(slug);

    if (existing_slug != impl_->workspace_id_by_slug.end() &&
        existing_slug->second != request.id)
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceAlreadyExists,
                                                               "Workspace slug is already used."});
    }

    impl_->workspace_id_by_slug.erase(item->second.slug);

    item->second.name = request.name;
    item->second.slug = slug;
    item->second.updated_at = now_timestamp();

    impl_->workspace_id_by_slug[item->second.slug] = item->second.id;

    return WorkspaceResult<dto::WorkspaceResponse>::success(item->second);
  }

  WorkspaceResult<dto::WorkspaceResponse> WorkspaceService::find_workspace(
      const dto::WorkspaceLookupRequest &request) const
  {
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, owner_user_id, name, slug, active, created_at, updated_at "
          "FROM workspaces WHERE id = ? LIMIT 1",
          request.id);

      if (!rows->next())
      {
        return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceNotFound,
                                                                 "Workspace not found."});
      }

      return WorkspaceResult<dto::WorkspaceResponse>::success(
          impl_->row_to_workspace(rows->row()));
    }

    auto item = impl_->workspaces_by_id.find(request.id);

    if (item == impl_->workspaces_by_id.end())
    {
      return WorkspaceResult<dto::WorkspaceResponse>::failure({support::WorkspaceErrorCode::WorkspaceNotFound,
                                                               "Workspace not found."});
    }

    return WorkspaceResult<dto::WorkspaceResponse>::success(item->second);
  }

  WorkspaceResult<std::vector<dto::WorkspaceResponse>> WorkspaceService::list_workspaces_for_owner(
      const std::string &owner_user_id) const
  {
    if (owner_user_id.empty())
    {
      return WorkspaceResult<std::vector<dto::WorkspaceResponse>>::failure({support::WorkspaceErrorCode::MissingOwner,
                                                                            "Workspace owner is required."});
    }

    std::vector<dto::WorkspaceResponse> workspaces;

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT DISTINCT w.id, w.owner_user_id, w.name, w.slug, w.active, w.created_at, w.updated_at, "
          "CASE WHEN w.owner_user_id = ? THEN 'owner' ELSE COALESCE(wm.role, 'viewer') END AS current_user_role, "
          "CASE WHEN w.owner_user_id = ? THEN 'active' ELSE COALESCE(wm.status, 'active') END AS current_user_status, "
          "CASE WHEN w.owner_user_id = ? THEN 'entire_workspace' ELSE COALESCE(wm.access_scope, 'entire_workspace') END AS access_scope, "
          "CASE WHEN w.owner_user_id = ? THEN '' ELSE COALESCE(wm.project_ids_json, '') END AS project_ids_json "
          "FROM workspaces w "
          "LEFT JOIN workspace_members wm ON wm.workspace_id = w.id AND wm.active = 1 AND wm.status = 'active' "
          "WHERE w.owner_user_id = ? OR wm.user_id = ? "
          "ORDER BY w.created_at",
          owner_user_id,
          owner_user_id,
          owner_user_id,
          owner_user_id,
          owner_user_id,
          owner_user_id);

      while (rows->next())
      {
        workspaces.push_back(impl_->row_to_workspace_with_role(rows->row()));
      }

      return WorkspaceResult<std::vector<dto::WorkspaceResponse>>::success(workspaces);
    }

    for (const auto &entry : impl_->workspaces_by_id)
    {
      if (entry.second.owner_user_id == owner_user_id)
      {
        auto workspace = entry.second;
        workspace.current_user_role = "owner";
        workspace.current_user_status = "active";
        workspace.access_scope = "entire_workspace";
        workspace.project_ids_json = "";
        workspace.current_user_is_owner = true;
        workspaces.push_back(workspace);
      }
    }

    std::sort(
        workspaces.begin(),
        workspaces.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return WorkspaceResult<std::vector<dto::WorkspaceResponse>>::success(workspaces);
  }
} // namespace cloud::workspaces::services
