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
#include <cctype>
#include <sstream>
#include <string>
#include <unordered_map>

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

    for (const auto &entry : impl_->workspaces_by_id)
    {
      if (entry.second.owner_user_id == owner_user_id)
      {
        workspaces.push_back(entry.second);
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
