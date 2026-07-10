/**
 *
 *  @file ProjectService.hpp
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
#ifndef cloud_projects_services_project_service_hpp
#define cloud_projects_services_project_service_hpp

#include <projects/dto/ProjectRequests.hpp>
#include <projects/dto/ProjectResponses.hpp>
#include <projects/support/ProjectErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::projects::services
{
  template <typename T>
  class ProjectResult
  {
  public:
    static ProjectResult success(T value)
    {
      ProjectResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static ProjectResult failure(
        support::ProjectError error)
    {
      ProjectResult result;
      result.ok_ = false;
      result.error_ = std::move(error);
      return result;
    }

    bool ok() const
    {
      return ok_;
    }

    bool failed() const
    {
      return !ok_;
    }

    const T &value() const
    {
      return value_;
    }

    const support::ProjectError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::ProjectError error_{};
  };

  class ProjectService
  {
  public:
    ProjectService();
    ~ProjectService();

    ProjectService(const ProjectService &) = delete;
    ProjectService &operator=(const ProjectService &) = delete;

    ProjectService(ProjectService &&) noexcept;
    ProjectService &operator=(ProjectService &&) noexcept;

    ProjectResult<dto::ProjectResponse> create_project(
        const dto::CreateProjectRequest &request);

    ProjectResult<dto::ProjectResponse> update_project(
        const dto::UpdateProjectRequest &request);

    ProjectResult<dto::ProjectResponse> find_project(
        const dto::ProjectLookupRequest &request) const;

    ProjectResult<dto::ProjectResponse> set_status(
        const dto::ProjectLookupRequest &request,
        const std::string &status);

    ProjectResult<std::vector<dto::ProjectResponse>> list_projects(
        const dto::ListProjectsRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::projects::services

#endif // cloud_projects_services_project_service_hpp
