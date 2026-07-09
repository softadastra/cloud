#ifndef cloud_workspace_invites_services_workspace_invite_service_hpp
#define cloud_workspace_invites_services_workspace_invite_service_hpp

#include <workspace_invites/dto/WorkspaceInviteRequests.hpp>
#include <workspace_invites/dto/WorkspaceInviteResponses.hpp>
#include <workspace_invites/support/WorkspaceInviteErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::workspace_invites::services
{
  template <typename T>
  class WorkspaceInviteResult
  {
  public:
    static WorkspaceInviteResult success(T value)
    {
      WorkspaceInviteResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static WorkspaceInviteResult failure(support::WorkspaceInviteError error)
    {
      WorkspaceInviteResult result;
      result.ok_ = false;
      result.error_ = std::move(error);
      return result;
    }

    bool ok() const { return ok_; }
    bool failed() const { return !ok_; }
    const T &value() const { return value_; }
    const support::WorkspaceInviteError &error() const { return error_; }

  private:
    bool ok_{false};
    T value_{};
    support::WorkspaceInviteError error_{};
  };

  class WorkspaceInviteService
  {
  public:
    WorkspaceInviteService();
    ~WorkspaceInviteService();
    WorkspaceInviteService(const WorkspaceInviteService &) = delete;
    WorkspaceInviteService &operator=(const WorkspaceInviteService &) = delete;
    WorkspaceInviteService(WorkspaceInviteService &&) noexcept;
    WorkspaceInviteService &operator=(WorkspaceInviteService &&) noexcept;

    WorkspaceInviteResult<dto::WorkspaceInviteResponse> create_invite(const dto::CreateWorkspaceInviteRequest &request);
    WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>> list_invites(const dto::ListWorkspaceInvitesRequest &request) const;
    WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>> list_my_invites(const dto::ListMyWorkspaceInvitesRequest &request) const;
    WorkspaceInviteResult<dto::WorkspaceInviteResponse> accept_invite(const dto::WorkspaceInviteActionRequest &request);
    WorkspaceInviteResult<dto::WorkspaceInviteResponse> decline_invite(const dto::WorkspaceInviteActionRequest &request);
    WorkspaceInviteResult<dto::WorkspaceInviteResponse> revoke_invite(const dto::RevokeWorkspaceInviteRequest &request);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
}

#endif
