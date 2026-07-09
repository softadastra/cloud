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
#include <sstream>
#include <string>
#include <unordered_map>

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
