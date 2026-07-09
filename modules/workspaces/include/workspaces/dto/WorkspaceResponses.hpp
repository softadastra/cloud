/**
 *
 *  @file WorkspaceResponses.hpp
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
#ifndef cloud_workspaces_dto_workspace_responses_hpp
#define cloud_workspaces_dto_workspace_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::workspaces::dto
{
  struct WorkspaceResponse
  {
    std::string id;
    std::string name;
    std::string slug;
    std::string owner_user_id;
    std::string current_user_role;
    bool current_user_is_owner{false};
    bool active{true};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "name", name,
          "slug", slug,
          "owner_user_id", owner_user_id,
          "current_user_role", current_user_role,
          "current_user_is_owner", current_user_is_owner,
          "active", active,
          "created_at", created_at,
          "updated_at", updated_at);
    }
  };
} // namespace cloud::workspaces::dto

#endif // cloud_workspaces_dto_workspace_responses_hpp
