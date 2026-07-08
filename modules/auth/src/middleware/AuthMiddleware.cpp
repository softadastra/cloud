/**
 *
 *  @file AuthMiddleware.cpp
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
#include <auth/middleware/AuthMiddleware.hpp>
#include <auth/services/AuthService.hpp>
#include <auth/support/AuthErrors.hpp>

namespace cloud::auth::middleware
{
  namespace
  {
    services::AuthService &auth_service()
    {
      static services::AuthService service;
      return service;
    }
  } // namespace

  rixlib::auth::AuthResult<rixlib::auth::Session> authenticate_session(
      const std::string &session_id)
  {
    return auth_service().authenticate_session(session_id);
  }

  bool require_session(
      const std::string &session_id,
      vix::Response &res)
  {
    auto session = authenticate_session(session_id);

    if (session.failed())
    {
      support::write_auth_error(res, session.error());
      return false;
    }

    return true;
  }
} // namespace cloud::auth::middleware
