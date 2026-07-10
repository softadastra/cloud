/**
 *
 *  @file LockfileResponses.hpp
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
#ifndef cloud_lockfiles_dto_lockfile_responses_hpp
#define cloud_lockfiles_dto_lockfile_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::lockfiles::dto
{
  struct LockfileResponse
  {
    std::string id;
    std::string workspace_id;
    std::string project_id;
    std::string uploaded_by_user_id;
    std::string lockfile_json;
    std::string checksum_sha256;
    std::string source;
    std::string status{"active"};
    std::int64_t deleted_at{0};
    std::int64_t created_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "project_id", project_id,
          "uploaded_by_user_id", uploaded_by_user_id,
          "lockfile_json", lockfile_json,
          "checksum_sha256", checksum_sha256,
          "source", source,
          "status", status,
          "deleted_at", deleted_at,
          "created_at", created_at);
    }
  };
} // namespace cloud::lockfiles::dto

#endif // cloud_lockfiles_dto_lockfile_responses_hpp
