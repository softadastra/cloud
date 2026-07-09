#ifndef cloud_workspace_invites_dto_workspace_invite_responses_hpp
#define cloud_workspace_invites_dto_workspace_invite_responses_hpp

#include <cstdint>
#include <string>
#include <vix/json.hpp>

namespace cloud::workspace_invites::dto
{
  struct WorkspaceInviteResponse
  {
    std::string id;
    std::string workspace_id;
    std::string workspace_name;
    std::string invited_email;
    std::string invited_user_id;
    std::string role;
    std::string invited_by_user_id;
    std::string status;
    std::int64_t created_at{0};
    std::int64_t updated_at{0};
    std::int64_t expires_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "workspace_name", workspace_name,
          "invited_email", invited_email,
          "invited_user_id", invited_user_id,
          "role", role,
          "invited_by_user_id", invited_by_user_id,
          "status", status,
          "created_at", created_at,
          "updated_at", updated_at,
          "expires_at", expires_at);
    }
  };
}

#endif
