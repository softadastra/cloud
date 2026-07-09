#ifndef cloud_workspace_invites_support_workspace_invite_errors_hpp
#define cloud_workspace_invites_support_workspace_invite_errors_hpp

#include <string>
#include <vix.hpp>

namespace cloud::workspace_invites::support
{
  enum class WorkspaceInviteErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingEmail,
    MissingInvite,
    MissingActor,
    InvalidRole,
    UserNotFound,
    WorkspaceInviteNotFound,
    WorkspaceInviteAlreadyExists,
    WorkspaceMemberAlreadyExists,
    WorkspaceInviteNotPending,
    Unauthorized,
    InternalError
  };

  struct WorkspaceInviteError
  {
    WorkspaceInviteErrorCode code{WorkspaceInviteErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_workspace_invite_error(const WorkspaceInviteError &error);
  std::string public_code_for_workspace_invite_error(const WorkspaceInviteError &error);
  std::string public_message_for_workspace_invite_error(const WorkspaceInviteError &error);
  void write_workspace_invite_error(vix::Response &res, const WorkspaceInviteError &error);
}

#endif
