/**
 *
 *  @file AuthService.cpp
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
#include <auth/services/AuthService.hpp>

namespace cloud::auth::services
{
  namespace
  {
    rixlib::auth::AuthConfig make_auth_config()
    {
      auto config = rix.auth.config.development();

      config.set_issuer("softadastra-cloud");
      config.set_require_email_verification(false);
      config.set_session_ttl_seconds(60 * 60 * 24 * 7);
      config.set_token_ttl_seconds(60 * 15);

      return config;
    }
  } // namespace

  class AuthService::Impl
  {
  public:
    Impl()
        : config_(make_auth_config()),
          auth_(rix.auth.memory(config_))
    {
    }

    rixlib::auth::AuthConfig config_;
    decltype(rix.auth.memory(config_)) auth_;
  };

  AuthService::AuthService()
      : impl_(std::make_unique<Impl>())
  {
  }

  AuthService::~AuthService() = default;

  AuthService::AuthService(AuthService &&) noexcept = default;

  AuthService &AuthService::operator=(AuthService &&) noexcept = default;

  rixlib::auth::AuthResult<rixlib::auth::User> AuthService::register_user(
      const dto::RegisterRequest &request)
  {
    return impl_->auth_.register_user({
        request.email,
        request.password,
    });
  }

  rixlib::auth::AuthResult<rixlib::auth::LoginResult> AuthService::login(
      const dto::LoginRequest &request)
  {
    return impl_->auth_.login({
        request.email,
        request.password,
    });
  }

  rixlib::auth::AuthResult<rixlib::auth::Session> AuthService::authenticate_session(
      const std::string &session_id)
  {
    return impl_->auth_.authenticate_session(session_id);
  }

  rixlib::auth::AuthResult<rixlib::auth::Session> AuthService::refresh_session(
      const std::string &session_id)
  {
    return impl_->auth_.refresh_session(session_id);
  }

  rixlib::auth::AuthStatus AuthService::logout(
      const std::string &session_id)
  {
    return impl_->auth_.logout(session_id);
  }

  rixlib::auth::AuthStatus AuthService::logout_user(
      const std::string &user_id)
  {
    return impl_->auth_.logout_user(user_id);
  }

  rixlib::auth::AuthResult<rixlib::auth::Token> AuthService::issue_token(
      const std::string &user_id)
  {
    return impl_->auth_.issue_token(user_id);
  }
} // namespace cloud::auth::services
