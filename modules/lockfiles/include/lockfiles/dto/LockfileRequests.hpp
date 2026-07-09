/**
 *
 *  @file LockfileRequests.hpp
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
#ifndef cloud_lockfiles_dto_lockfile_requests_hpp
#define cloud_lockfiles_dto_lockfile_requests_hpp

#include <string>

namespace cloud::lockfiles::dto
{
  struct UploadLockfileRequest
  {
    std::string workspace_id;
    std::string project_id;
    std::string uploaded_by_user_id;
    std::string lockfile_json;
    std::string checksum_sha256;
    std::string source;
  };

  struct LockfileLookupRequest
  {
    std::string workspace_id;
    std::string project_id;
    std::string lockfile_id;
  };

  struct ListLockfilesRequest
  {
    std::string workspace_id;
    std::string project_id;
  };
} // namespace cloud::lockfiles::dto

#endif // cloud_lockfiles_dto_lockfile_requests_hpp
