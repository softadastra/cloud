/**
 *
 *  @file LockfileErrors.hpp
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
#ifndef cloud_lockfiles_support_lockfile_errors_hpp
#define cloud_lockfiles_support_lockfile_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::lockfiles::support
{
  enum class LockfileErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingProject,
    MissingUploader,
    MissingLockfile,
    InvalidChecksum,
    LockfileNotFound,
    Unauthorized,
    InternalError
  };

  struct LockfileError
  {
    LockfileErrorCode code{LockfileErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_lockfile_error(
      const LockfileError &error);

  std::string public_code_for_lockfile_error(
      const LockfileError &error);

  std::string public_message_for_lockfile_error(
      const LockfileError &error);

  void write_lockfile_error(
      vix::Response &res,
      const LockfileError &error);
} // namespace cloud::lockfiles::support

#endif // cloud_lockfiles_support_lockfile_errors_hpp
