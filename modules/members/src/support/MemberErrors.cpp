/**
 *
 *  @file MemberErrors.cpp
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
#include <members/support/MemberErrors.hpp>

#include <vix/json.hpp>

namespace cloud::members::support
{
  int http_status_for_member_error(
      const MemberError &error)
  {
    switch (error.code)
    {
    case MemberErrorCode::InvalidRequest:
    case MemberErrorCode::MissingWorkspace:
    case MemberErrorCode::MissingUser:
    case MemberErrorCode::MissingEmail:
    case MemberErrorCode::InvalidRole:
      return 400;

    case MemberErrorCode::Unauthorized:
      return 401;

    case MemberErrorCode::MemberNotFound:
    case MemberErrorCode::UserNotFound:
      return 404;

    case MemberErrorCode::MemberAlreadyExists:
      return 409;

    case MemberErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_member_error(
      const MemberError &error)
  {
    switch (error.code)
    {
    case MemberErrorCode::InvalidRequest:
      return "invalid_request";

    case MemberErrorCode::MissingWorkspace:
      return "missing_workspace";

    case MemberErrorCode::MissingUser:
      return "missing_user";

    case MemberErrorCode::MissingEmail:
      return "missing_email";

    case MemberErrorCode::InvalidRole:
      return "invalid_member_role";

    case MemberErrorCode::Unauthorized:
      return "unauthorized";

    case MemberErrorCode::MemberNotFound:
      return "member_not_found";

    case MemberErrorCode::UserNotFound:
      return "user_not_found";

    case MemberErrorCode::MemberAlreadyExists:
      return "workspace_member_already_exists";

    case MemberErrorCode::InternalError:
    default:
      return "member_error";
    }
  }

  std::string public_message_for_member_error(
      const MemberError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case MemberErrorCode::InvalidRequest:
      return "Invalid member request.";

    case MemberErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case MemberErrorCode::MissingUser:
      return "User is required.";

    case MemberErrorCode::MissingEmail:
      return "Email is required.";

    case MemberErrorCode::InvalidRole:
      return "Invalid member role.";

    case MemberErrorCode::Unauthorized:
      return "Authentication is required.";

    case MemberErrorCode::MemberNotFound:
      return "Member not found.";

    case MemberErrorCode::UserNotFound:
      return "User not found for this email";

    case MemberErrorCode::MemberAlreadyExists:
      return "Workspace member already exists.";

    case MemberErrorCode::InternalError:
    default:
      return "Member operation failed.";
    }
  }

  void write_member_error(
      vix::Response &res,
      const MemberError &error)
  {
    res.status(http_status_for_member_error(error)).json(vix::json::o("ok", false, "error", public_code_for_member_error(error), "message", public_message_for_member_error(error)));
  }
} // namespace cloud::members::support
