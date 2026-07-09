/**
 *
 *  @file PackageRequests.hpp
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
#ifndef cloud_packages_dto_package_requests_hpp
#define cloud_packages_dto_package_requests_hpp

#include <string>

namespace cloud::packages::dto
{
  struct CreatePackageRequest
  {
    std::string workspace_id;
    std::string owner_user_id;
    std::string name;
    std::string description;
    std::string repository_url;
    std::string visibility;
  };

  struct UpdatePackageRequest
  {
    std::string id;
    std::string workspace_id;
    std::string name;
    std::string description;
    std::string repository_url;
    std::string visibility;
  };

  struct PackageLookupRequest
  {
    std::string id;
    std::string workspace_id;
  };

  struct ListPackagesRequest
  {
    std::string workspace_id;
  };
} // namespace cloud::packages::dto

#endif // cloud_packages_dto_package_requests_hpp
