/**
 *
 *  @file LockfileErrors.cpp
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
#include <lockfiles/support/LockfileErrors.hpp>

#include <vix/json.hpp>

namespace cloud::lockfiles::support
{
  int http_status_for_lockfile_error(
      const LockfileError &error)
  {
    switch (error.code)
    {
    case LockfileErrorCode::InvalidRequest:
    case LockfileErrorCode::MissingWorkspace:
    case LockfileErrorCode::MissingProject:
    case LockfileErrorCode::MissingUploader:
    case LockfileErrorCode::MissingLockfile:
    case LockfileErrorCode::InvalidChecksum:
      return 400;

    case LockfileErrorCode::Unauthorized:
      return 401;

    case LockfileErrorCode::LockfileNotFound:
      return 404;

    case LockfileErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_lockfile_error(
      const LockfileError &error)
  {
    switch (error.code)
    {
    case LockfileErrorCode::InvalidRequest:
      return "invalid_request";

    case LockfileErrorCode::MissingWorkspace:
      return "missing_workspace";

    case LockfileErrorCode::MissingProject:
      return "missing_project";

    case LockfileErrorCode::MissingUploader:
      return "missing_uploader";

    case LockfileErrorCode::MissingLockfile:
      return "missing_lockfile";

    case LockfileErrorCode::InvalidChecksum:
      return "invalid_checksum";

    case LockfileErrorCode::Unauthorized:
      return "unauthorized";

    case LockfileErrorCode::LockfileNotFound:
      return "lockfile_not_found";

    case LockfileErrorCode::InternalError:
    default:
      return "lockfile_error";
    }
  }

  std::string public_message_for_lockfile_error(
      const LockfileError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case LockfileErrorCode::InvalidRequest:
      return "Invalid lockfile request.";

    case LockfileErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case LockfileErrorCode::MissingProject:
      return "Project is required.";

    case LockfileErrorCode::MissingUploader:
      return "Uploader is required.";

    case LockfileErrorCode::MissingLockfile:
      return "Lockfile content is required.";

    case LockfileErrorCode::InvalidChecksum:
      return "Invalid lockfile checksum.";

    case LockfileErrorCode::Unauthorized:
      return "Authentication is required.";

    case LockfileErrorCode::LockfileNotFound:
      return "Lockfile not found.";

    case LockfileErrorCode::InternalError:
    default:
      return "Lockfile operation failed.";
    }
  }

  void write_lockfile_error(
      vix::Response &res,
      const LockfileError &error)
  {
    res.status(http_status_for_lockfile_error(error)).json(vix::json::o("ok", false, "error", public_code_for_lockfile_error(error), "message", public_message_for_lockfile_error(error)));
  }
} // namespace cloud::lockfiles::support
