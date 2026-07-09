/**
 *
 *  @file PackageVersionErrors.cpp
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
#include <package_versions/support/PackageVersionErrors.hpp>

#include <vix/json.hpp>

namespace cloud::package_versions::support
{
  int http_status_for_package_version_error(
      const PackageVersionError &error)
  {
    switch (error.code)
    {
    case PackageVersionErrorCode::InvalidRequest:
    case PackageVersionErrorCode::MissingWorkspace:
    case PackageVersionErrorCode::MissingPackage:
    case PackageVersionErrorCode::MissingPublisher:
    case PackageVersionErrorCode::InvalidVersion:
    case PackageVersionErrorCode::MissingArchive:
    case PackageVersionErrorCode::InvalidChecksum:
    case PackageVersionErrorCode::InvalidManifest:
    case PackageVersionErrorCode::InvalidSize:
      return 400;

    case PackageVersionErrorCode::Unauthorized:
      return 401;

    case PackageVersionErrorCode::PackageVersionNotFound:
      return 404;

    case PackageVersionErrorCode::PackageVersionAlreadyExists:
      return 409;

    case PackageVersionErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_package_version_error(
      const PackageVersionError &error)
  {
    switch (error.code)
    {
    case PackageVersionErrorCode::InvalidRequest:
      return "invalid_request";

    case PackageVersionErrorCode::MissingWorkspace:
      return "missing_workspace";

    case PackageVersionErrorCode::MissingPackage:
      return "missing_package";

    case PackageVersionErrorCode::MissingPublisher:
      return "missing_publisher";

    case PackageVersionErrorCode::InvalidVersion:
      return "invalid_package_version";

    case PackageVersionErrorCode::MissingArchive:
      return "missing_archive";

    case PackageVersionErrorCode::InvalidChecksum:
      return "invalid_checksum";

    case PackageVersionErrorCode::InvalidManifest:
      return "invalid_manifest";

    case PackageVersionErrorCode::InvalidSize:
      return "invalid_size";

    case PackageVersionErrorCode::Unauthorized:
      return "unauthorized";

    case PackageVersionErrorCode::PackageVersionNotFound:
      return "package_version_not_found";

    case PackageVersionErrorCode::PackageVersionAlreadyExists:
      return "package_version_already_exists";

    case PackageVersionErrorCode::InternalError:
    default:
      return "package_version_error";
    }
  }

  std::string public_message_for_package_version_error(
      const PackageVersionError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case PackageVersionErrorCode::InvalidRequest:
      return "Invalid package version request.";

    case PackageVersionErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case PackageVersionErrorCode::MissingPackage:
      return "Package is required.";

    case PackageVersionErrorCode::MissingPublisher:
      return "Publisher is required.";

    case PackageVersionErrorCode::InvalidVersion:
      return "Invalid package version.";

    case PackageVersionErrorCode::MissingArchive:
      return "Package archive is required.";

    case PackageVersionErrorCode::InvalidChecksum:
      return "Invalid package checksum.";

    case PackageVersionErrorCode::InvalidManifest:
      return "Invalid package manifest.";

    case PackageVersionErrorCode::InvalidSize:
      return "Invalid package archive size.";

    case PackageVersionErrorCode::Unauthorized:
      return "Authentication is required.";

    case PackageVersionErrorCode::PackageVersionNotFound:
      return "Package version not found.";

    case PackageVersionErrorCode::PackageVersionAlreadyExists:
      return "Package version already exists.";

    case PackageVersionErrorCode::InternalError:
    default:
      return "Package version operation failed.";
    }
  }

  void write_package_version_error(
      vix::Response &res,
      const PackageVersionError &error)
  {
    res.status(http_status_for_package_version_error(error)).json(vix::json::o("ok", false, "error", public_code_for_package_version_error(error), "message", public_message_for_package_version_error(error)));
  }
} // namespace cloud::package_versions::support
