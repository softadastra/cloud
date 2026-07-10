/**
 *
 *  @file PackageVersionResponses.hpp
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
#ifndef cloud_package_versions_dto_package_version_responses_hpp
#define cloud_package_versions_dto_package_version_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::package_versions::dto
{
  struct PackageVersionResponse
  {
    std::string id;
    std::string workspace_id;
    std::string package_id;
    std::string published_by_user_id;
    std::string version;
    std::string archive_url;
    std::string checksum_sha256;
    std::string manifest_json;
    std::string status;
    std::int64_t size_bytes{0};
    std::int64_t yanked_at{0};
    std::int64_t deprecated_at{0};
    std::string deprecation_message;
    std::int64_t deleted_at{0};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "package_id", package_id,
          "published_by_user_id", published_by_user_id,
          "version", version,
          "archive_url", archive_url,
          "checksum_sha256", checksum_sha256,
          "manifest_json", manifest_json,
          "status", status,
          "size_bytes", size_bytes,
          "yanked_at", yanked_at,
          "deprecated_at", deprecated_at,
          "deprecation_message", deprecation_message,
          "deleted_at", deleted_at,
          "created_at", created_at,
          "updated_at", updated_at);
    }
  };
} // namespace cloud::package_versions::dto

#endif // cloud_package_versions_dto_package_version_responses_hpp
