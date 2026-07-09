/**
 *
 *  @file PackageVersionErrors.hpp
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
#ifndef cloud_package_versions_support_package_version_errors_hpp
#define cloud_package_versions_support_package_version_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::package_versions::support
{
  enum class PackageVersionErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingPackage,
    MissingPublisher,
    InvalidVersion,
    MissingArchive,
    InvalidChecksum,
    InvalidManifest,
    InvalidSize,
    PackageVersionNotFound,
    PackageVersionAlreadyExists,
    Unauthorized,
    InternalError
  };

  struct PackageVersionError
  {
    PackageVersionErrorCode code{PackageVersionErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_package_version_error(
      const PackageVersionError &error);

  std::string public_code_for_package_version_error(
      const PackageVersionError &error);

  std::string public_message_for_package_version_error(
      const PackageVersionError &error);

  void write_package_version_error(
      vix::Response &res,
      const PackageVersionError &error);
} // namespace cloud::package_versions::support

#endif // cloud_package_versions_support_package_version_errors_hpp
