/**
 *
 *  @file TokenErrors.cpp
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
#include <tokens/support/TokenErrors.hpp>

#include <vix/json.hpp>

namespace cloud::tokens::support
{
  int http_status_for_token_error(
      const TokenError &error)
  {
    switch (error.code)
    {
    case TokenErrorCode::InvalidRequest:
    case TokenErrorCode::MissingWorkspace:
    case TokenErrorCode::MissingUser:
    case TokenErrorCode::MissingName:
    case TokenErrorCode::InvalidScopes:
      return 400;

    case TokenErrorCode::Unauthorized:
      return 401;

    case TokenErrorCode::TokenNotFound:
      return 404;

    case TokenErrorCode::TokenAlreadyRevoked:
      return 409;

    case TokenErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_token_error(
      const TokenError &error)
  {
    switch (error.code)
    {
    case TokenErrorCode::InvalidRequest:
      return "invalid_request";

    case TokenErrorCode::MissingWorkspace:
      return "missing_workspace";

    case TokenErrorCode::MissingUser:
      return "missing_user";

    case TokenErrorCode::MissingName:
      return "missing_token_name";

    case TokenErrorCode::InvalidScopes:
      return "invalid_token_scopes";

    case TokenErrorCode::Unauthorized:
      return "unauthorized";

    case TokenErrorCode::TokenNotFound:
      return "token_not_found";

    case TokenErrorCode::TokenAlreadyRevoked:
      return "token_already_revoked";

    case TokenErrorCode::InternalError:
    default:
      return "token_error";
    }
  }

  std::string public_message_for_token_error(
      const TokenError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case TokenErrorCode::InvalidRequest:
      return "Invalid token request.";

    case TokenErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case TokenErrorCode::MissingUser:
      return "User is required.";

    case TokenErrorCode::MissingName:
      return "Token name is required.";

    case TokenErrorCode::InvalidScopes:
      return "Token scopes are invalid.";

    case TokenErrorCode::Unauthorized:
      return "Authentication is required.";

    case TokenErrorCode::TokenNotFound:
      return "Token not found.";

    case TokenErrorCode::TokenAlreadyRevoked:
      return "Token is already revoked.";

    case TokenErrorCode::InternalError:
    default:
      return "Token operation failed.";
    }
  }

  void write_token_error(
      vix::Response &res,
      const TokenError &error)
  {
    res.status(http_status_for_token_error(error)).json(vix::json::o("ok", false, "error", public_code_for_token_error(error), "message", public_message_for_token_error(error)));
  }
} // namespace cloud::tokens::support
