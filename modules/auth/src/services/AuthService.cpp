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

#include <cstdlib>
#include <memory>
#include <string>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

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

    bool should_use_database()
    {
      const char *use_db = std::getenv("SOFTADASTRA_CLOUD_USE_DB");
      return use_db != nullptr && std::string(use_db) == "1";
    }
  } // namespace

  class AuthService::Impl
  {
  public:
    Impl()
        : config_(make_auth_config())
    {
      if (should_use_database())
      {
        vix::config::Config cfg{".env"};
        db_ = std::make_unique<vix::db::Database>(cfg);
        auth_ = std::make_unique<rixlib::auth::ManagedAuth>(
            rix.auth.database(*db_, config_));
        return;
      }

      auth_ = std::make_unique<rixlib::auth::ManagedAuth>(
          rix.auth.memory(config_));
    }

    rixlib::auth::ManagedAuth &auth()
    {
      return *auth_;
    }

    rixlib::auth::AuthConfig config_;
    std::unique_ptr<vix::db::Database> db_;
    std::unique_ptr<rixlib::auth::ManagedAuth> auth_;
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
    return impl_->auth().register_user({
        request.email,
        request.password,
    });
  }

  rixlib::auth::AuthResult<rixlib::auth::LoginResult> AuthService::login(
      const dto::LoginRequest &request)
  {
    return impl_->auth().login({
        request.email,
        request.password,
    });
  }

  rixlib::auth::AuthResult<rixlib::auth::Session> AuthService::authenticate_session(
      const std::string &session_id)
  {
    return impl_->auth().authenticate_session(session_id);
  }

  rixlib::auth::AuthResult<rixlib::auth::Session> AuthService::refresh_session(
      const std::string &session_id)
  {
    return impl_->auth().refresh_session(session_id);
  }

  rixlib::auth::AuthStatus AuthService::logout(
      const std::string &session_id)
  {
    return impl_->auth().logout(session_id);
  }

  rixlib::auth::AuthStatus AuthService::logout_user(
      const std::string &user_id)
  {
    return impl_->auth().logout_user(user_id);
  }

  rixlib::auth::AuthResult<rixlib::auth::Token> AuthService::issue_token(
      const std::string &user_id)
  {
    return impl_->auth().issue_token(user_id);
  }
} // namespace cloud::auth::services
