/**
 *
 *  @file AuthMiddleware.hpp
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
#ifndef cloud_auth_middleware_auth_middleware_hpp
#define cloud_auth_middleware_auth_middleware_hpp

#include <string>

#include <rix.hpp>
#include <vix.hpp>

namespace cloud::auth::middleware
{
  struct AuthContext
  {
    std::string user_id;
    std::string workspace_id;
    std::string role;
    std::string status;
    std::string access_scope;
    std::string project_ids_json;
    bool cli_token{false};
  };

  using WorkspacePermissionResult = bool;

  bool require_api_auth_and_workspace_permission(
      vix::Request &req,
      vix::Response &res);

  rixlib::auth::AuthResult<rixlib::auth::Session> authenticate_session(
      const std::string &session_id);

  bool require_session(
      const std::string &session_id,
      vix::Response &res);
} // namespace cloud::auth::middleware

#endif // cloud_auth_middleware_auth_middleware_hpp
