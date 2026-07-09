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

#include <chrono>

#include <cstdlib>
#include <memory>
#include <string>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::auth::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    rixlib::auth::AuthError auth_error(
        rixlib::auth::AuthErrorCode code,
        const std::string &message)
    {
      return rixlib::auth::make_auth_error(code, message);
    }

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

    bool persistent() const
    {
      return db_ != nullptr;
    }

    void ensure_display_name_column()
    {
      if (!persistent() || display_name_checked_)
      {
        return;
      }

      display_name_checked_ = true;

      try
      {
        auto rows = db_->query("PRAGMA table_info(rix_auth_users)");

        while (rows->next())
        {
          if (rows->row().getString(1) == "display_name")
          {
            return;
          }
        }

        db_->exec("ALTER TABLE rix_auth_users ADD COLUMN display_name TEXT");
      }
      catch (...)
      {
      }
    }

    rixlib::auth::AuthResult<dto::AuthUserResponse> find_user_profile(
        const std::string &user_id)
    {
      if (!persistent())
      {
        return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
            auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
      }

      ensure_display_name_column();

      auto rows = db_->query(
          "SELECT id, email, email_verified, active, created_at, COALESCE(display_name, '') "
          "FROM rix_auth_users WHERE id = ? LIMIT 1",
          user_id);

      if (!rows->next())
      {
        return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
            auth_error(rixlib::auth::AuthErrorCode::UserNotFound, "User not found."));
      }

      const auto &row = rows->row();
      dto::AuthUserResponse user;
      user.id = row.getString(0);
      user.email = row.getString(1);
      user.email_verified = row.getInt64(2) != 0;
      user.active = row.getInt64(3) != 0;
      user.created_at = row.getInt64(4);
      user.display_name = row.getString(5);
      user.name = user.display_name.empty() ? user.email : user.display_name;

      return rixlib::auth::AuthResult<dto::AuthUserResponse>::success(user);
    }

    rixlib::auth::AuthConfig config_;
    std::unique_ptr<vix::db::Database> db_;
    std::unique_ptr<rixlib::auth::ManagedAuth> auth_;
    bool display_name_checked_{false};
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


namespace cloud::auth::services
{
  rixlib::auth::AuthResult<dto::AuthUserResponse> AuthService::user_profile(
      const std::string &user_id)
  {
    return impl_->find_user_profile(user_id);
  }

  rixlib::auth::AuthResult<dto::AuthUserResponse> AuthService::update_profile(
      const dto::UpdateProfileRequest &request)
  {
    auto session = authenticate_session(request.session_id);

    if (session.failed())
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(session.error());
    }

    if (request.display_name.size() > 120)
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidInput, "Display name is too long."));
    }

    if (!impl_->persistent())
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
    }

    impl_->ensure_display_name_column();
    impl_->db_->exec(
        "UPDATE rix_auth_users SET display_name = ?, updated_at = ? WHERE id = ?",
        request.display_name,
        now_timestamp(),
        session.value().user_id());

    return impl_->find_user_profile(session.value().user_id());
  }

  rixlib::auth::AuthStatus AuthService::change_password(
      const dto::ChangePasswordRequest &request)
  {
    auto session = authenticate_session(request.session_id);

    if (session.failed())
    {
      return rixlib::auth::AuthStatus::failure(session.error());
    }

    if (request.new_password != request.confirm_new_password)
    {
      return rixlib::auth::AuthStatus::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidPassword, "New password confirmation does not match."));
    }

    if (!impl_->persistent())
    {
      return rixlib::auth::AuthStatus::failure(
          auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
    }

    auto rows = impl_->db_->query(
        "SELECT password_hash FROM rix_auth_users WHERE id = ? LIMIT 1",
        session.value().user_id());

    if (!rows->next())
    {
      return rixlib::auth::AuthStatus::failure(
          auth_error(rixlib::auth::AuthErrorCode::UserNotFound, "User not found."));
    }

    const auto password_hash = rows->row().getString(0);

    if (!impl_->auth().password_hasher().verify(request.current_password, password_hash))
    {
      return rixlib::auth::AuthStatus::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidCredentials, "Current password is invalid."));
    }

    auto next_hash = impl_->auth().password_hasher().hash(request.new_password);

    if (next_hash.failed())
    {
      return rixlib::auth::AuthStatus::failure(next_hash.error());
    }

    impl_->db_->exec(
        "UPDATE rix_auth_users SET password_hash = ?, updated_at = ? WHERE id = ?",
        next_hash.value(),
        now_timestamp(),
        session.value().user_id());

    return rixlib::auth::AuthStatus::success();
  }
} // namespace cloud::auth::services
