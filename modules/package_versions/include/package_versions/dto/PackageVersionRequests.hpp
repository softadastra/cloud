/**
 *
 *  @file PackageVersionRequests.hpp
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
#ifndef cloud_package_versions_dto_package_version_requests_hpp
#define cloud_package_versions_dto_package_version_requests_hpp

#include <cstdint>
#include <string>

namespace cloud::package_versions::dto
{
  struct PublishPackageVersionRequest
  {
    std::string workspace_id;
    std::string package_id;
    std::string published_by_user_id;
    std::string version;
    std::string archive_url;
    std::string checksum_sha256;
    std::string manifest_json;
    std::int64_t size_bytes{0};
  };

  struct PackageVersionLookupRequest
  {
    std::string workspace_id;
    std::string package_id;
    std::string version_id;
  };

  struct PackageVersionByNumberRequest
  {
    std::string workspace_id;
    std::string package_id;
    std::string version;
  };

  struct ListPackageVersionsRequest
  {
    std::string workspace_id;
    std::string package_id;
  };
} // namespace cloud::package_versions::dto

#endif // cloud_package_versions_dto_package_version_requests_hpp
