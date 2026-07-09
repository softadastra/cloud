#ifndef cloud_workspace_invites_dto_workspace_invite_requests_hpp
#define cloud_workspace_invites_dto_workspace_invite_requests_hpp

#include <cstdint>
#include <string>

namespace cloud::workspace_invites::dto
{
  struct CreateWorkspaceInviteRequest
  {
    std::string workspace_id;
    std::string invited_email;
    std::string role;
    std::string invited_by_user_id;
    std::string access_scope;
    std::string project_ids_json;
    std::int64_t expires_at{0};
  };

  struct ListWorkspaceInvitesRequest
  {
    std::string workspace_id;
  };

  struct ListMyWorkspaceInvitesRequest
  {
    std::string user_id;
    std::string email;
  };

  struct WorkspaceInviteActionRequest
  {
    std::string invite_id;
    std::string user_id;
    std::string email;
  };

  struct RevokeWorkspaceInviteRequest
  {
    std::string workspace_id;
    std::string invite_id;
    std::string revoked_by_user_id;
  };
}

#endif
