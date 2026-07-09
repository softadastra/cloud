/**
 *
 *  @file ProjectErrors.cpp
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
#include <projects/support/ProjectErrors.hpp>

#include <vix/json.hpp>

namespace cloud::projects::support
{
  int http_status_for_project_error(
      const ProjectError &error)
  {
    switch (error.code)
    {
    case ProjectErrorCode::InvalidRequest:
    case ProjectErrorCode::MissingWorkspace:
    case ProjectErrorCode::MissingOwner:
    case ProjectErrorCode::InvalidName:
    case ProjectErrorCode::InvalidSlug:
      return 400;

    case ProjectErrorCode::Unauthorized:
      return 401;

    case ProjectErrorCode::ProjectNotFound:
      return 404;

    case ProjectErrorCode::ProjectAlreadyExists:
      return 409;

    case ProjectErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_project_error(
      const ProjectError &error)
  {
    switch (error.code)
    {
    case ProjectErrorCode::InvalidRequest:
      return "invalid_request";

    case ProjectErrorCode::MissingWorkspace:
      return "missing_workspace";

    case ProjectErrorCode::MissingOwner:
      return "missing_owner";

    case ProjectErrorCode::InvalidName:
      return "invalid_project_name";

    case ProjectErrorCode::InvalidSlug:
      return "invalid_project_slug";

    case ProjectErrorCode::Unauthorized:
      return "unauthorized";

    case ProjectErrorCode::ProjectNotFound:
      return "project_not_found";

    case ProjectErrorCode::ProjectAlreadyExists:
      return "project_already_exists";

    case ProjectErrorCode::InternalError:
    default:
      return "project_error";
    }
  }

  std::string public_message_for_project_error(
      const ProjectError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case ProjectErrorCode::InvalidRequest:
      return "Invalid project request.";

    case ProjectErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case ProjectErrorCode::MissingOwner:
      return "Project owner is required.";

    case ProjectErrorCode::InvalidName:
      return "Invalid project name.";

    case ProjectErrorCode::InvalidSlug:
      return "Invalid project slug.";

    case ProjectErrorCode::Unauthorized:
      return "Authentication is required.";

    case ProjectErrorCode::ProjectNotFound:
      return "Project not found.";

    case ProjectErrorCode::ProjectAlreadyExists:
      return "Project already exists.";

    case ProjectErrorCode::InternalError:
    default:
      return "Project operation failed.";
    }
  }

  void write_project_error(
      vix::Response &res,
      const ProjectError &error)
  {
    res.status(http_status_for_project_error(error)).json(vix::json::o("ok", false, "error", public_code_for_project_error(error), "message", public_message_for_project_error(error)));
  }
} // namespace cloud::projects::support
