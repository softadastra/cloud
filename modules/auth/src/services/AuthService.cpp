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

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <sstream>
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

    std::string make_profile_id()
    {
      static std::atomic<std::uint64_t> counter{0};
      std::ostringstream out;
      out << "profile_" << now_timestamp() << "_" << counter.fetch_add(1);
      return out.str();
    }

    std::string trim_copy(const std::string &value)
    {
      const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
      });

      const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
      }).base();

      if (begin >= end)
      {
        return {};
      }

      return std::string(begin, end);
    }

    std::string normalize_username(const std::string &value)
    {
      auto username = trim_copy(value);

      std::transform(username.begin(), username.end(), username.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });

      return username;
    }

    bool valid_username(const std::string &username)
    {
      if (username.empty())
      {
        return true;
      }

      if (username.size() < 3 || username.size() > 32)
      {
        return false;
      }

      return std::all_of(username.begin(), username.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '-' || ch == '_';
      });
    }

    bool valid_optional_url(const std::string &value)
    {
      if (value.empty())
      {
        return true;
      }

      return value.rfind("https://", 0) == 0 || value.rfind("http://", 0) == 0;
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

    void ensure_user_profiles_table()
    {
      if (!persistent() || user_profiles_checked_)
      {
        return;
      }

      user_profiles_checked_ = true;

      db_->exec(
          "CREATE TABLE IF NOT EXISTS user_profiles ("
          "id TEXT PRIMARY KEY, "
          "user_id TEXT NOT NULL UNIQUE, "
          "display_name TEXT, "
          "username TEXT UNIQUE, "
          "bio TEXT, "
          "avatar_url TEXT, "
          "avatar_storage_path TEXT, "
          "avatar_updated_at INTEGER, "
          "website_url TEXT, "
          "github_url TEXT, "
          "public_profile_enabled INTEGER NOT NULL DEFAULT 0, "
          "created_at INTEGER NOT NULL, "
          "updated_at INTEGER NOT NULL, "
          "FOREIGN KEY(user_id) REFERENCES rix_auth_users(id) ON DELETE CASCADE)");
      db_->exec("CREATE INDEX IF NOT EXISTS idx_user_profiles_user_id ON user_profiles(user_id)");
      db_->exec("CREATE INDEX IF NOT EXISTS idx_user_profiles_username ON user_profiles(username)");
    }

    std::string legacy_display_name(const std::string &user_id)
    {
      try
      {
        auto info = db_->query("PRAGMA table_info(rix_auth_users)");
        bool has_display_name = false;

        while (info->next())
        {
          if (info->row().getString(1) == "display_name")
          {
            has_display_name = true;
            break;
          }
        }

        if (!has_display_name)
        {
          return {};
        }

        auto rows = db_->query(
            "SELECT COALESCE(display_name, '') FROM rix_auth_users WHERE id = ? LIMIT 1",
            user_id);

        return rows->next() ? rows->row().getString(0) : std::string{};
      }
      catch (...)
      {
        return {};
      }
    }

    void ensure_profile_for_user(
        const std::string &user_id,
        const std::string &display_name = {})
    {
      if (!persistent() || user_id.empty())
      {
        return;
      }

      ensure_user_profiles_table();

      auto exists = db_->query(
          "SELECT id FROM user_profiles WHERE user_id = ? LIMIT 1",
          user_id);

      if (exists->next())
      {
        return;
      }

      const auto now = now_timestamp();
      const auto initial_name = display_name.empty() ? legacy_display_name(user_id) : display_name;

      db_->exec(
          "INSERT INTO user_profiles "
          "(id, user_id, display_name, username, bio, avatar_url, avatar_storage_path, avatar_updated_at, website_url, github_url, public_profile_enabled, created_at, updated_at) "
          "VALUES (?, ?, NULLIF(?, ''), NULL, '', NULL, NULL, NULL, '', '', 0, ?, ?)",
          make_profile_id(),
          user_id,
          initial_name,
          now,
          now);
    }

    rixlib::auth::AuthResult<dto::AuthUserResponse> find_user_profile(
        const std::string &user_id)
    {
      if (!persistent())
      {
        return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
            auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
      }

      ensure_profile_for_user(user_id);

      auto rows = db_->query(
          "SELECT u.id, u.email, u.email_verified, u.active, u.created_at, "
          "COALESCE(p.display_name, ''), COALESCE(p.username, ''), COALESCE(p.bio, ''), "
          "COALESCE(p.avatar_url, ''), COALESCE(p.website_url, ''), COALESCE(p.github_url, ''), "
          "COALESCE(p.public_profile_enabled, 0) "
          "FROM rix_auth_users u "
          "LEFT JOIN user_profiles p ON p.user_id = u.id "
          "WHERE u.id = ? LIMIT 1",
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
      user.username = row.getString(6);
      user.bio = row.getString(7);
      user.avatar_url = row.getString(8);
      user.website_url = row.getString(9);
      user.github_url = row.getString(10);
      user.public_profile_enabled = row.getInt64(11) != 0;
      user.name = user.display_name.empty() ? user.email : user.display_name;

      return rixlib::auth::AuthResult<dto::AuthUserResponse>::success(user);
    }

    void create_public_activity_event(
        const std::string &user_id,
        const std::string &type,
        const std::string &title,
        const std::string &data_json)
    {
      ensure_user_profiles_table();
      db_->exec(
          "CREATE TABLE IF NOT EXISTS public_activity_events ("
          "id TEXT PRIMARY KEY, "
          "user_id TEXT NOT NULL, "
          "workspace_id TEXT, "
          "project_id TEXT, "
          "package_id TEXT, "
          "type TEXT NOT NULL, "
          "title TEXT NOT NULL, "
          "data_json TEXT, "
          "visibility TEXT NOT NULL DEFAULT 'public', "
          "created_at INTEGER NOT NULL)");

      const auto now = now_timestamp();
      db_->exec(
          "INSERT INTO public_activity_events "
          "(id, user_id, workspace_id, project_id, package_id, type, title, data_json, visibility, created_at) "
          "VALUES (?, ?, NULL, NULL, NULL, ?, ?, ?, 'public', ?)",
          std::string{"activity_"} + make_profile_id(),
          user_id,
          type,
          title,
          data_json,
          now);
    }

    bool username_taken(const std::string &user_id, const std::string &username)
    {
      if (username.empty())
      {
        return false;
      }

      auto rows = db_->query(
          "SELECT user_id FROM user_profiles WHERE username = ? AND user_id <> ? LIMIT 1",
          username,
          user_id);

      return rows->next();
    }

    std::string avatar_storage_path(const std::string &user_id)
    {
      ensure_profile_for_user(user_id);

      auto rows = db_->query(
          "SELECT COALESCE(avatar_storage_path, '') FROM user_profiles WHERE user_id = ? LIMIT 1",
          user_id);

      return rows->next() ? rows->row().getString(0) : std::string{};
    }

    rixlib::auth::AuthConfig config_;
    std::unique_ptr<vix::db::Database> db_;
    std::unique_ptr<rixlib::auth::ManagedAuth> auth_;
    bool user_profiles_checked_{false};
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
    auto registered = impl_->auth().register_user({
        request.email,
        request.password,
    });

    if (registered.failed())
    {
      return registered;
    }

    impl_->ensure_profile_for_user(registered.value().id(), trim_copy(request.name));

    return registered;
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

    if (!impl_->persistent())
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
    }

    const auto user_id = session.value().user_id();
    const auto display_name = trim_copy(request.display_name);
    const auto username = normalize_username(request.username);
    const auto bio = trim_copy(request.bio);
    const auto website_url = trim_copy(request.website_url);
    const auto github_url = trim_copy(request.github_url);

    if (display_name.size() > 80)
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidInput, "Display name is too long."));
    }

    if (!valid_username(username))
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidInput, "username_invalid"));
    }

    if (bio.size() > 500)
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidInput, "Bio is too long."));
    }

    if (website_url.size() > 200 || github_url.size() > 200 ||
        !valid_optional_url(website_url) || !valid_optional_url(github_url))
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::InvalidInput, "Profile URL is invalid."));
    }

    impl_->ensure_profile_for_user(user_id);

    if (impl_->username_taken(user_id, username))
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::ValidationError, "username_already_taken"));
    }

    impl_->db_->exec(
        "UPDATE user_profiles "
        "SET display_name = NULLIF(?, ''), username = NULLIF(?, ''), bio = ?, website_url = ?, github_url = ?, public_profile_enabled = ?, updated_at = ? "
        "WHERE user_id = ?",
        display_name,
        username,
        bio,
        website_url,
        github_url,
        request.public_profile_enabled ? static_cast<std::int64_t>(1) : static_cast<std::int64_t>(0),
        now_timestamp(),
        user_id);

    if (request.public_profile_enabled && !username.empty())
    {
      impl_->create_public_activity_event(
          user_id,
          "profile_updated",
          "Profile updated",
          "{}");
    }

    return impl_->find_user_profile(user_id);
  }

  rixlib::auth::AuthResult<dto::AuthUserResponse> AuthService::update_avatar(
      const std::string &session_id,
      const std::string &avatar_url,
      const std::string &avatar_storage_path)
  {
    auto session = authenticate_session(session_id);

    if (session.failed())
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(session.error());
    }

    if (!impl_->persistent())
    {
      return rixlib::auth::AuthResult<dto::AuthUserResponse>::failure(
          auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
    }

    const auto user_id = session.value().user_id();
    impl_->ensure_profile_for_user(user_id);

    impl_->db_->exec(
        "UPDATE user_profiles "
        "SET avatar_url = ?, avatar_storage_path = ?, avatar_updated_at = ?, updated_at = ? "
        "WHERE user_id = ?",
        avatar_url,
        avatar_storage_path,
        now_timestamp(),
        now_timestamp(),
        user_id);

    return impl_->find_user_profile(user_id);
  }

  rixlib::auth::AuthResult<std::string> AuthService::delete_avatar(
      const std::string &session_id)
  {
    auto session = authenticate_session(session_id);

    if (session.failed())
    {
      return rixlib::auth::AuthResult<std::string>::failure(session.error());
    }

    if (!impl_->persistent())
    {
      return rixlib::auth::AuthResult<std::string>::failure(
          auth_error(rixlib::auth::AuthErrorCode::StoreError, "Persistent auth storage is required."));
    }

    const auto user_id = session.value().user_id();
    const auto previous_path = impl_->avatar_storage_path(user_id);

    impl_->db_->exec(
        "UPDATE user_profiles "
        "SET avatar_url = NULL, avatar_storage_path = NULL, avatar_updated_at = NULL, updated_at = ? "
        "WHERE user_id = ?",
        now_timestamp(),
        user_id);

    return rixlib::auth::AuthResult<std::string>::success(previous_path);
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
