/**
 *
 *  @file WorkspaceRequests.hpp
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
#ifndef cloud_workspaces_dto_workspace_requests_hpp
#define cloud_workspaces_dto_workspace_requests_hpp

#include <string>

namespace cloud::workspaces::dto
{
  struct CreateWorkspaceRequest
  {
    std::string name;
    std::string slug;
    std::string owner_user_id;
  };

  struct UpdateWorkspaceRequest
  {
    std::string id;
    std::string name;
    std::string slug;
  };

  struct WorkspaceLookupRequest
  {
    std::string id;
  };
} // namespace cloud::workspaces::dto

#endif // cloud_workspaces_dto_workspace_requests_hpp
