/**
 *
 *  @file ProjectRequests.hpp
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
#ifndef cloud_projects_dto_project_requests_hpp
#define cloud_projects_dto_project_requests_hpp

#include <string>

namespace cloud::projects::dto
{
  struct CreateProjectRequest
  {
    std::string workspace_id;
    std::string owner_user_id;
    std::string name;
    std::string slug;
    std::string description;
    std::string repository_url;
    std::string default_branch;
  };

  struct UpdateProjectRequest
  {
    std::string id;
    std::string workspace_id;
    std::string name;
    std::string slug;
    std::string description;
    std::string repository_url;
    std::string default_branch;
  };

  struct ProjectLookupRequest
  {
    std::string id;
    std::string workspace_id;
  };

  struct ListProjectsRequest
  {
    std::string workspace_id;
    std::string access_scope;
    std::string project_ids_json;
  };
} // namespace cloud::projects::dto

#endif // cloud_projects_dto_project_requests_hpp
