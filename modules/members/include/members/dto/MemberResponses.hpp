/**
 *
 *  @file MemberResponses.hpp
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
#ifndef cloud_members_dto_member_responses_hpp
#define cloud_members_dto_member_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::members::dto
{
  struct MemberResponse
  {
    std::string id;
    std::string workspace_id;
    std::string user_id;
    std::string email;
    std::string role;
    std::string status;
    std::string access_scope;
    std::string project_ids_json;
    std::string invited_by_user_id;
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "user_id", user_id,
          "email", email,
          "role", role,
          "status", status,
          "access_scope", access_scope,
          "project_ids_json", project_ids_json,
          "invited_by_user_id", invited_by_user_id,
          "created_at", created_at,
          "updated_at", updated_at);
    }
  };
} // namespace cloud::members::dto

#endif // cloud_members_dto_member_responses_hpp
