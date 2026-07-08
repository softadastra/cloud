/**
 *
 *  @file WorkspaceErrors.cpp
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
#include <workspaces/support/WorkspaceErrors.hpp>

#include <vix/json.hpp>

namespace cloud::workspaces::support
{
  int http_status_for_workspace_error(
      const WorkspaceError &error)
  {
    switch (error.code)
    {
    case WorkspaceErrorCode::InvalidRequest:
    case WorkspaceErrorCode::InvalidName:
    case WorkspaceErrorCode::InvalidSlug:
    case WorkspaceErrorCode::MissingOwner:
      return 400;

    case WorkspaceErrorCode::Unauthorized:
      return 401;

    case WorkspaceErrorCode::WorkspaceNotFound:
      return 404;

    case WorkspaceErrorCode::WorkspaceAlreadyExists:
      return 409;

    case WorkspaceErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_workspace_error(
      const WorkspaceError &error)
  {
    switch (error.code)
    {
    case WorkspaceErrorCode::InvalidRequest:
      return "invalid_request";

    case WorkspaceErrorCode::InvalidName:
      return "invalid_workspace_name";

    case WorkspaceErrorCode::InvalidSlug:
      return "invalid_workspace_slug";

    case WorkspaceErrorCode::MissingOwner:
      return "missing_workspace_owner";

    case WorkspaceErrorCode::Unauthorized:
      return "unauthorized";

    case WorkspaceErrorCode::WorkspaceNotFound:
      return "workspace_not_found";

    case WorkspaceErrorCode::WorkspaceAlreadyExists:
      return "workspace_already_exists";

    case WorkspaceErrorCode::InternalError:
    default:
      return "workspace_error";
    }
  }

  std::string public_message_for_workspace_error(
      const WorkspaceError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case WorkspaceErrorCode::InvalidRequest:
      return "Invalid workspace request.";

    case WorkspaceErrorCode::InvalidName:
      return "Invalid workspace name.";

    case WorkspaceErrorCode::InvalidSlug:
      return "Invalid workspace slug.";

    case WorkspaceErrorCode::MissingOwner:
      return "Workspace owner is required.";

    case WorkspaceErrorCode::Unauthorized:
      return "Authentication is required.";

    case WorkspaceErrorCode::WorkspaceNotFound:
      return "Workspace not found.";

    case WorkspaceErrorCode::WorkspaceAlreadyExists:
      return "Workspace already exists.";

    case WorkspaceErrorCode::InternalError:
    default:
      return "Workspace operation failed.";
    }
  }

  void write_workspace_error(
      vix::Response &res,
      const WorkspaceError &error)
  {
    res.status(http_status_for_workspace_error(error)).json(vix::json::o("ok", false, "error", public_code_for_workspace_error(error), "message", public_message_for_workspace_error(error)));
  }
} // namespace cloud::workspaces::support
