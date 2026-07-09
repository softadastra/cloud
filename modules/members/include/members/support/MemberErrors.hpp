/**
 *
 *  @file MemberErrors.hpp
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
#ifndef cloud_members_support_member_errors_hpp
#define cloud_members_support_member_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::members::support
{
  enum class MemberErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingUser,
    MissingEmail,
    InvalidRole,
    InvalidStatus,
    MemberNotFound,
    UserNotFound,
    MemberAlreadyExists,
    CannotModifyOwner,
    CannotRemoveOwner,
    PermissionDenied,
    Unauthorized,
    InternalError
  };

  struct MemberError
  {
    MemberErrorCode code{MemberErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_member_error(
      const MemberError &error);

  std::string public_code_for_member_error(
      const MemberError &error);

  std::string public_message_for_member_error(
      const MemberError &error);

  void write_member_error(
      vix::Response &res,
      const MemberError &error);
} // namespace cloud::members::support

#endif // cloud_members_support_member_errors_hpp
