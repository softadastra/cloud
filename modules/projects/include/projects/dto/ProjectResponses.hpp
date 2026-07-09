/**
 *
 *  @file ProjectResponses.hpp
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
#ifndef cloud_projects_dto_project_responses_hpp
#define cloud_projects_dto_project_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::projects::dto
{
  struct ProjectResponse
  {
    std::string id;
    std::string workspace_id;
    std::string owner_user_id;
    std::string name;
    std::string slug;
    std::string description;
    std::string repository_url;
    std::string default_branch;
    bool active{true};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "owner_user_id", owner_user_id,
          "name", name,
          "slug", slug,
          "description", description,
          "repository_url", repository_url,
          "default_branch", default_branch,
          "active", active,
          "created_at", created_at,
          "updated_at", updated_at);
    }
  };
} // namespace cloud::projects::dto

#endif // cloud_projects_dto_project_responses_hpp
