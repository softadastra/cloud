/**
 *
 *  @file WorkspaceService.hpp
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
#ifndef cloud_workspaces_services_workspace_service_hpp
#define cloud_workspaces_services_workspace_service_hpp

#include <workspaces/dto/WorkspaceRequests.hpp>
#include <workspaces/dto/WorkspaceResponses.hpp>
#include <workspaces/support/WorkspaceErrors.hpp>

#include <memory>
#include <vector>

namespace cloud::workspaces::services
{
  template <typename T>
  class WorkspaceResult
  {
  public:
    static WorkspaceResult success(T value)
    {
      WorkspaceResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static WorkspaceResult failure(
        support::WorkspaceError error)
    {
      WorkspaceResult result;
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

    const support::WorkspaceError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::WorkspaceError error_{};
  };

  class WorkspaceService
  {
  public:
    WorkspaceService();
    ~WorkspaceService();

    WorkspaceService(const WorkspaceService &) = delete;
    WorkspaceService &operator=(const WorkspaceService &) = delete;

    WorkspaceService(WorkspaceService &&) noexcept;
    WorkspaceService &operator=(WorkspaceService &&) noexcept;

    WorkspaceResult<dto::WorkspaceResponse> create_workspace(
        const dto::CreateWorkspaceRequest &request);

    WorkspaceResult<dto::WorkspaceResponse> update_workspace(
        const dto::UpdateWorkspaceRequest &request);

    WorkspaceResult<dto::WorkspaceResponse> find_workspace(
        const dto::WorkspaceLookupRequest &request) const;

    WorkspaceResult<dto::WorkspaceResponse> update_avatar(
        const std::string &workspace_id,
        const std::string &avatar_url,
        const std::string &avatar_storage_path);

    WorkspaceResult<std::string> delete_avatar(
        const std::string &workspace_id);

    WorkspaceResult<dto::WorkspaceResponse> set_status(
        const std::string &workspace_id,
        const std::string &status);

    WorkspaceResult<std::vector<dto::WorkspaceResponse>> list_workspaces_for_owner(
        const std::string &owner_user_id) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::workspaces::services

#endif // cloud_workspaces_services_workspace_service_hpp
