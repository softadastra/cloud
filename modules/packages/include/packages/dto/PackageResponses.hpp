/**
 *
 *  @file PackageResponses.hpp
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
#ifndef cloud_packages_dto_package_responses_hpp
#define cloud_packages_dto_package_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::packages::dto
{
  struct PackageResponse
  {
    std::string id;
    std::string workspace_id;
    std::string owner_user_id;
    std::string name;
    std::string description;
    std::string repository_url;
    std::string visibility;
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
          "description", description,
          "repository_url", repository_url,
          "visibility", visibility,
          "active", active,
          "created_at", created_at,
          "updated_at", updated_at);
    }
  };
} // namespace cloud::packages::dto

#endif // cloud_packages_dto_package_responses_hpp
