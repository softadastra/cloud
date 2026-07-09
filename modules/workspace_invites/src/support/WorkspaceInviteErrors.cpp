#include <workspace_invites/support/WorkspaceInviteErrors.hpp>

#include <vix/json.hpp>

namespace cloud::workspace_invites::support
{
  int http_status_for_workspace_invite_error(const WorkspaceInviteError &error)
  {
    switch (error.code)
    {
    case WorkspaceInviteErrorCode::InvalidRequest:
    case WorkspaceInviteErrorCode::MissingWorkspace:
    case WorkspaceInviteErrorCode::MissingEmail:
    case WorkspaceInviteErrorCode::MissingInvite:
    case WorkspaceInviteErrorCode::MissingActor:
    case WorkspaceInviteErrorCode::InvalidRole:
      return 400;
    case WorkspaceInviteErrorCode::Unauthorized:
      return 401;
    case WorkspaceInviteErrorCode::UserNotFound:
    case WorkspaceInviteErrorCode::WorkspaceInviteNotFound:
      return 404;
    case WorkspaceInviteErrorCode::WorkspaceInviteAlreadyExists:
    case WorkspaceInviteErrorCode::WorkspaceMemberAlreadyExists:
    case WorkspaceInviteErrorCode::WorkspaceInviteNotPending:
      return 409;
    case WorkspaceInviteErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_workspace_invite_error(const WorkspaceInviteError &error)
  {
    switch (error.code)
    {
    case WorkspaceInviteErrorCode::InvalidRequest:
      return "invalid_request";
    case WorkspaceInviteErrorCode::MissingWorkspace:
      return "missing_workspace";
    case WorkspaceInviteErrorCode::MissingEmail:
      return "missing_email";
    case WorkspaceInviteErrorCode::MissingInvite:
      return "missing_invite";
    case WorkspaceInviteErrorCode::MissingActor:
      return "missing_actor";
    case WorkspaceInviteErrorCode::InvalidRole:
      return "invalid_member_role";
    case WorkspaceInviteErrorCode::Unauthorized:
      return "unauthorized";
    case WorkspaceInviteErrorCode::UserNotFound:
      return "user_not_found";
    case WorkspaceInviteErrorCode::WorkspaceInviteNotFound:
      return "workspace_invite_not_found";
    case WorkspaceInviteErrorCode::WorkspaceInviteAlreadyExists:
      return "workspace_invite_already_exists";
    case WorkspaceInviteErrorCode::WorkspaceMemberAlreadyExists:
      return "workspace_member_already_exists";
    case WorkspaceInviteErrorCode::WorkspaceInviteNotPending:
      return "workspace_invite_not_pending";
    case WorkspaceInviteErrorCode::InternalError:
    default:
      return "workspace_invite_error";
    }
  }

  std::string public_message_for_workspace_invite_error(const WorkspaceInviteError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case WorkspaceInviteErrorCode::UserNotFound:
      return "User not found for this email";
    case WorkspaceInviteErrorCode::WorkspaceInviteAlreadyExists:
      return "Workspace invite already exists.";
    case WorkspaceInviteErrorCode::WorkspaceMemberAlreadyExists:
      return "Workspace member already exists.";
    case WorkspaceInviteErrorCode::WorkspaceInviteNotPending:
      return "Workspace invite is not pending.";
    case WorkspaceInviteErrorCode::WorkspaceInviteNotFound:
      return "Workspace invite not found.";
    case WorkspaceInviteErrorCode::MissingWorkspace:
      return "Workspace is required.";
    case WorkspaceInviteErrorCode::MissingEmail:
      return "Email is required.";
    case WorkspaceInviteErrorCode::MissingInvite:
      return "Invite is required.";
    case WorkspaceInviteErrorCode::MissingActor:
      return "Actor is required.";
    case WorkspaceInviteErrorCode::InvalidRole:
      return "Invalid member role.";
    case WorkspaceInviteErrorCode::Unauthorized:
      return "Authentication is required.";
    case WorkspaceInviteErrorCode::InvalidRequest:
      return "Invalid workspace invite request.";
    case WorkspaceInviteErrorCode::InternalError:
    default:
      return "Workspace invite operation failed.";
    }
  }

  void write_workspace_invite_error(vix::Response &res, const WorkspaceInviteError &error)
  {
    res.status(http_status_for_workspace_invite_error(error)).json(vix::json::o(
        "ok", false,
        "error", public_code_for_workspace_invite_error(error),
        "message", public_message_for_workspace_invite_error(error)));
  }
}
