/**
 *
 *  @file AuthService.hpp
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
#ifndef cloud_auth_services_auth_service_hpp
#define cloud_auth_services_auth_service_hpp

#include <auth/dto/AuthRequests.hpp>
#include <auth/dto/AuthResponses.hpp>

#include <memory>
#include <string>

#include <rix.hpp>

namespace cloud::auth::services
{
  class AuthService
  {
  public:
    AuthService();
    ~AuthService();

    AuthService(const AuthService &) = delete;
    AuthService &operator=(const AuthService &) = delete;

    AuthService(AuthService &&) noexcept;
    AuthService &operator=(AuthService &&) noexcept;

    rixlib::auth::AuthResult<rixlib::auth::User> register_user(
        const dto::RegisterRequest &request);

    rixlib::auth::AuthResult<rixlib::auth::LoginResult> login(
        const dto::LoginRequest &request);

    rixlib::auth::AuthResult<rixlib::auth::Session> authenticate_session(
        const std::string &session_id);

    rixlib::auth::AuthResult<rixlib::auth::Session> refresh_session(
        const std::string &session_id);

    rixlib::auth::AuthStatus logout(
        const std::string &session_id);

    rixlib::auth::AuthStatus logout_user(
        const std::string &user_id);

    rixlib::auth::AuthResult<rixlib::auth::Token> issue_token(
        const std::string &user_id);

    rixlib::auth::AuthResult<dto::AuthUserResponse> update_profile(
        const dto::UpdateProfileRequest &request);

    rixlib::auth::AuthStatus change_password(
        const dto::ChangePasswordRequest &request);

    rixlib::auth::AuthResult<dto::AuthUserResponse> user_profile(
        const std::string &user_id);

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::auth::services

#endif // cloud_auth_services_auth_service_hpp
