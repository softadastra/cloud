/**
 *
 *  @file AuthErrors.cpp
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
#include <auth/support/AuthErrors.hpp>

namespace cloud::auth::support
{
  int http_status_for_auth_error(
      const rixlib::auth::AuthError &error)
  {
    (void)error;

    return 401;
  }

  std::string public_code_for_auth_error(
      const rixlib::auth::AuthError &error)
  {
    (void)error;

    return "auth_error";
  }

  std::string public_message_for_auth_error(
      const rixlib::auth::AuthError &error)
  {
    const auto message = error.message();

    if (!message.empty())
    {
      return message;
    }

    return "Authentication failed.";
  }

  void write_auth_error(
      vix::Response &res,
      const rixlib::auth::AuthError &error)
  {
    res.status(http_status_for_auth_error(error)).json(vix::json::o("ok", false, "error", public_code_for_auth_error(error), "message", public_message_for_auth_error(error)));
  }
} // namespace cloud::auth::support
