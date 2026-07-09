/**
 *
 *  @file TokenErrors.hpp
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
#ifndef cloud_tokens_support_token_errors_hpp
#define cloud_tokens_support_token_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::tokens::support
{
  enum class TokenErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingUser,
    MissingName,
    InvalidScopes,
    TokenNotFound,
    TokenAlreadyRevoked,
    Unauthorized,
    InternalError
  };

  struct TokenError
  {
    TokenErrorCode code{TokenErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_token_error(
      const TokenError &error);

  std::string public_code_for_token_error(
      const TokenError &error);

  std::string public_message_for_token_error(
      const TokenError &error);

  void write_token_error(
      vix::Response &res,
      const TokenError &error);
} // namespace cloud::tokens::support

#endif // cloud_tokens_support_token_errors_hpp
