/**
 *
 *  @file WorkspaceErrors.hpp
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
#ifndef cloud_workspaces_support_workspace_errors_hpp
#define cloud_workspaces_support_workspace_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::workspaces::support
{
  enum class WorkspaceErrorCode
  {
    InvalidRequest,
    InvalidName,
    InvalidSlug,
    MissingOwner,
    WorkspaceNotFound,
    WorkspaceAlreadyExists,
    Unauthorized,
    InternalError
  };

  struct WorkspaceError
  {
    WorkspaceErrorCode code{WorkspaceErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_workspace_error(
      const WorkspaceError &error);

  std::string public_code_for_workspace_error(
      const WorkspaceError &error);

  std::string public_message_for_workspace_error(
      const WorkspaceError &error);

  void write_workspace_error(
      vix::Response &res,
      const WorkspaceError &error);
} // namespace cloud::workspaces::support

#endif // cloud_workspaces_support_workspace_errors_hpp
