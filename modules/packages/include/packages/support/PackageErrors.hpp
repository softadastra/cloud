/**
 *
 *  @file PackageErrors.hpp
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
#ifndef cloud_packages_support_package_errors_hpp
#define cloud_packages_support_package_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::packages::support
{
  enum class PackageErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingOwner,
    InvalidName,
    InvalidVisibility,
    PackageNotFound,
    PackageAlreadyExists,
    Unauthorized,
    InternalError
  };

  struct PackageError
  {
    PackageErrorCode code{PackageErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_package_error(
      const PackageError &error);

  std::string public_code_for_package_error(
      const PackageError &error);

  std::string public_message_for_package_error(
      const PackageError &error);

  void write_package_error(
      vix::Response &res,
      const PackageError &error);
} // namespace cloud::packages::support

#endif // cloud_packages_support_package_errors_hpp
