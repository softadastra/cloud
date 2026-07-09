/**
 *
 *  @file ProjectErrors.hpp
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
#ifndef cloud_projects_support_project_errors_hpp
#define cloud_projects_support_project_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::projects::support
{
  enum class ProjectErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingOwner,
    InvalidName,
    InvalidSlug,
    ProjectNotFound,
    ProjectAlreadyExists,
    Unauthorized,
    InternalError
  };

  struct ProjectError
  {
    ProjectErrorCode code{ProjectErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_project_error(
      const ProjectError &error);

  std::string public_code_for_project_error(
      const ProjectError &error);

  std::string public_message_for_project_error(
      const ProjectError &error);

  void write_project_error(
      vix::Response &res,
      const ProjectError &error);
} // namespace cloud::projects::support

#endif // cloud_projects_support_project_errors_hpp
