/**
 *
 *  @file PackageErrors.cpp
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
#include <packages/support/PackageErrors.hpp>
#include <vix/json.hpp>

namespace cloud::packages::support
{
  int http_status_for_package_error(
      const PackageError &error)
  {
    switch (error.code)
    {
    case PackageErrorCode::InvalidRequest:
    case PackageErrorCode::MissingWorkspace:
    case PackageErrorCode::MissingOwner:
    case PackageErrorCode::InvalidName:
    case PackageErrorCode::InvalidVisibility:
      return 400;

    case PackageErrorCode::Unauthorized:
      return 401;

    case PackageErrorCode::PackageNotFound:
      return 404;

    case PackageErrorCode::PackageAlreadyExists:
      return 409;

    case PackageErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_package_error(
      const PackageError &error)
  {
    switch (error.code)
    {
    case PackageErrorCode::InvalidRequest:
      return "invalid_request";

    case PackageErrorCode::MissingWorkspace:
      return "missing_workspace";

    case PackageErrorCode::MissingOwner:
      return "missing_owner";

    case PackageErrorCode::InvalidName:
      return "invalid_package_name";

    case PackageErrorCode::InvalidVisibility:
      return "invalid_package_visibility";

    case PackageErrorCode::Unauthorized:
      return "unauthorized";

    case PackageErrorCode::PackageNotFound:
      return "package_not_found";

    case PackageErrorCode::PackageAlreadyExists:
      return "package_already_exists";

    case PackageErrorCode::InternalError:
    default:
      return "package_error";
    }
  }

  std::string public_message_for_package_error(
      const PackageError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case PackageErrorCode::InvalidRequest:
      return "Invalid package request.";

    case PackageErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case PackageErrorCode::MissingOwner:
      return "Package owner is required.";

    case PackageErrorCode::InvalidName:
      return "Invalid package name.";

    case PackageErrorCode::InvalidVisibility:
      return "Invalid package visibility.";

    case PackageErrorCode::Unauthorized:
      return "Authentication is required.";

    case PackageErrorCode::PackageNotFound:
      return "Package not found.";

    case PackageErrorCode::PackageAlreadyExists:
      return "Package already exists.";

    case PackageErrorCode::InternalError:
    default:
      return "Package operation failed.";
    }
  }

  void write_package_error(
      vix::Response &res,
      const PackageError &error)
  {
    res.status(http_status_for_package_error(error)).json(vix::json::o("ok", false, "error", public_code_for_package_error(error), "message", public_message_for_package_error(error)));
  }
} // namespace cloud::packages::support
