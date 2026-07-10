/**
 *
 *  @file AuthController.cpp
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
#include <auth/controllers/AuthController.hpp>

#include <auth/dto/AuthRequests.hpp>
#include <auth/services/AuthService.hpp>

#include <rix/auth/AuthError.hpp>
#include <auth/support/AuthErrors.hpp>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vix.hpp>

namespace cloud::auth::controllers
{
  namespace
  {
    void json_error(
        vix::Response &res,
        int status,
        const std::string &code,
        const std::string &message)
    {
      res.status(status).json(
          vix::json::o(
              "ok", false,
              "error", code,
              "message", message));
    }

    void json_ok(
        vix::Response &res,
        const vix::json::Json &data)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", data));
    }

    void json_message(
        vix::Response &res,
        const std::string &message)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "message", message)));
    }

    services::AuthService &auth_service()
    {
      static services::AuthService service;
      return service;
    }

    dto::RegisterRequest read_register_request(
        const vix::json::Json &body)
    {
      return {
          body.value("name", ""),
          body.value("email", ""),
          body.value("password", ""),
      };
    }

    dto::LoginRequest read_login_request(
        const vix::json::Json &body)
    {
      return {
          body.value("email", ""),
          body.value("password", ""),
      };
    }

    std::string read_session_id(
        const vix::json::Json &body)
    {
      return body.value("session_id", "");
    }

    dto::UpdateProfileRequest read_update_profile_request(
        const vix::json::Json &body)
    {
      return {
          body.value("session_id", ""),
          body.value("display_name", ""),
          body.value("username", ""),
          body.value("bio", ""),
          body.value("website_url", ""),
          body.value("github_url", ""),
          body.value("public_profile_enabled", false),
      };
    }

    dto::ChangePasswordRequest read_change_password_request(
        const vix::json::Json &body)
    {
      return {
          body.value("session_id", ""),
          body.value("current_password", ""),
          body.value("new_password", ""),
          body.value("confirm_new_password", ""),
      };
    }

    void write_account_error(
        vix::Response &res,
        const rixlib::auth::AuthError &error)
    {
      using rixlib::auth::AuthErrorCode;

      if (error.is(AuthErrorCode::InvalidSession) ||
          error.is(AuthErrorCode::SessionExpired) ||
          error.is(AuthErrorCode::SessionRevoked))
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      if (error.is(AuthErrorCode::InvalidCredentials))
      {
        json_error(res, 403, "current_password_invalid", "Current password is invalid.");
        return;
      }

      if (error.is(AuthErrorCode::InvalidPassword))
      {
        json_error(res, 400, "password_too_weak", error.message().empty() ? "Password is too weak." : error.message());
        return;
      }

      if (error.is(AuthErrorCode::InvalidInput) || error.is(AuthErrorCode::ValidationError))
      {
        if (error.message() == "username_invalid")
        {
          json_error(res, 400, "username_invalid", "Username must be 3 to 32 characters and use only letters, numbers, hyphen or underscore.");
          return;
        }

        if (error.message() == "username_already_taken")
        {
          json_error(res, 409, "username_already_taken", "This username is already taken.");
          return;
        }

        json_error(res, 400, "invalid_request", error.message().empty() ? "Invalid request." : error.message());
        return;
      }

      if (error.is(AuthErrorCode::UserNotFound))
      {
        json_error(res, 404, "user_not_found", "User not found.");
        return;
      }

      json_error(res, 500, "auth_error", error.message().empty() ? "Authentication service error." : error.message());
    }

    bool require_json_object(
        const vix::json::Json &body,
        vix::Response &res)
    {
      if (body.is_object())
      {
        return true;
      }

      json_error(
          res,
          400,
          "invalid_request",
          "Expected JSON object body.");

      return false;
    }

    std::string bearer_session_id(const vix::Request &req)
    {
      auto session_id = req.header("X-Session-Id");

      if (session_id.empty())
      {
        session_id = req.header("x-session-id");
      }

      if (!session_id.empty())
      {
        return session_id;
      }

      auto authorization = req.header("Authorization");

      if (authorization.empty())
      {
        authorization = req.header("authorization");
      }

      const std::string prefix = "Bearer ";

      if (authorization.rfind(prefix, 0) == 0)
      {
        return authorization.substr(prefix.size());
      }

      return {};
    }

    bool safe_path_segment(const std::string &value)
    {
      if (value.empty())
      {
        return false;
      }

      for (const auto ch : value)
      {
        const auto c = static_cast<unsigned char>(ch);

        if (std::isalnum(c) == 0 && ch != '_' && ch != '-' && ch != '.')
        {
          return false;
        }
      }

      return value != "." && value != "..";
    }

    std::string avatar_extension_for_content_type(const std::string &content_type)
    {
      if (content_type.rfind("image/jpeg", 0) == 0)
      {
        return ".jpg";
      }

      if (content_type.rfind("image/png", 0) == 0)
      {
        return ".png";
      }

      if (content_type.rfind("image/webp", 0) == 0)
      {
        return ".webp";
      }

      return {};
    }

    std::string content_type_for_avatar_filename(const std::string &filename)
    {
      if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".jpg")
      {
        return "image/jpeg";
      }

      if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".png")
      {
        return "image/png";
      }

      if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".webp")
      {
        return "image/webp";
      }

      return "application/octet-stream";
    }
  } // namespace

  void AuthController::register_routes(vix::App &app)
  {
    app.get("/api/auth", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Auth module is available"); });

    app.post("/api/auth/register", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_register_request(body);
      auto registered = auth_service().register_user(request);

      if (registered.failed())
      {
        support::write_auth_error(res, registered.error());
        return;
      }

      const auto &user = registered.value();
      auto profile = auth_service().user_profile(user.id());

      if (profile.failed())
      {
        write_account_error(res, profile.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "user", profile.value().to_json()))); });

    app.post("/api/auth/login", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_login_request(body);
      auto login = auth_service().login(request);

      if (login.failed())
      {
        support::write_auth_error(res, login.error());
        return;
      }

      const auto &result = login.value();
      auto profile = auth_service().user_profile(result.user.id());

      if (profile.failed())
      {
        write_account_error(res, profile.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "user", profile.value().to_json(),
              "session", vix::json::o(
                  "id", result.session.id(),
                  "expires_at", result.session.expires_at()),
              "token", vix::json::o(
                  "value", result.token.value(),
                  "issuer", result.token.issuer(),
                  "expires_at", result.token.expires_at()))); });

    app.post("/api/auth/logout", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto session_id = read_session_id(body);
      auto logout = auth_service().logout(session_id);

      if (logout.failed())
      {
        support::write_auth_error(res, logout.error());
        return;
      }

      json_message(
          res,
          "Logged out successfully."); });

    app.post("/api/auth/me", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto session_id = read_session_id(body);
      auto session = auth_service().authenticate_session(session_id);

      if (session.failed())
      {
        support::write_auth_error(res, session.error());
        return;
      }

      auto profile = auth_service().user_profile(session.value().user_id());

      if (profile.failed())
      {
        write_account_error(res, profile.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "user", profile.value().to_json(),
              "session", vix::json::o(
                  "id", session.value().id(),
                  "user_id", session.value().user_id(),
                  "expires_at", session.value().expires_at(),
                  "last_seen_at", session.value().last_seen_at(),
                  "revoked", session.value().revoked()))); });

    app.post("/api/auth/update_profile", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto updated = auth_service().update_profile(read_update_profile_request(body));

      if (updated.failed())
      {
        write_account_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o("user", updated.value().to_json())); });

    app.post("/api/auth/avatar/upload", [](vix::Request &req, vix::Response &res)
             {
      const auto session_id = bearer_session_id(req);

      if (session_id.empty())
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      auto session = auth_service().authenticate_session(session_id);

      if (session.failed())
      {
        write_account_error(res, session.error());
        return;
      }

      auto content_type = req.header("Content-Type");

      if (content_type.empty())
      {
        content_type = req.header("content-type");
      }

      const auto extension = avatar_extension_for_content_type(content_type);

      if (extension.empty())
      {
        json_error(res, 400, "invalid_content_type", "Avatar must be a JPG, PNG or WebP image.");
        return;
      }

      const auto &body = req.body();
      constexpr std::size_t max_avatar_size = 2u * 1024u * 1024u;

      if (body.size() > max_avatar_size)
      {
        json_error(res, 413, "avatar_too_large", "Avatar image must be 2MB or smaller.");
        return;
      }

      if (body.empty())
      {
        json_error(res, 400, "avatar_upload_failed", "Avatar image body is required.");
        return;
      }

      const auto user_id = session.value().user_id();
      const std::filesystem::path storage_root{"storage/users"};
      const auto avatar_dir = storage_root / user_id;
      const auto filename = std::string{"avatar"} + extension;
      const auto avatar_path = avatar_dir / filename;

      try
      {
        std::filesystem::create_directories(avatar_dir);

        for (const auto &entry : std::filesystem::directory_iterator(avatar_dir))
        {
          if (entry.is_regular_file() && entry.path().stem() == "avatar")
          {
            std::filesystem::remove(entry.path());
          }
        }

        std::ofstream out(avatar_path, std::ios::binary | std::ios::trunc);

        if (!out.good())
        {
          json_error(res, 500, "avatar_upload_failed", "Could not write avatar image.");
          return;
        }

        out.write(body.data(), static_cast<std::streamsize>(body.size()));
      }
      catch (...)
      {
        json_error(res, 500, "avatar_upload_failed", "Could not write avatar image.");
        return;
      }

      const auto avatar_url = std::string{"/storage/users/"} + user_id + "/" + filename;
      auto updated = auth_service().update_avatar(session_id, avatar_url, avatar_path.string());

      if (updated.failed())
      {
        write_account_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o(
          "avatar_url", avatar_url,
          "user", updated.value().to_json())); });

    app.post("/api/auth/avatar/delete", [](vix::Request &req, vix::Response &res)
             {
      const auto session_id = bearer_session_id(req);

      if (session_id.empty())
      {
        const auto &body = req.json();

        if (body.is_object())
        {
          auto deleted = auth_service().delete_avatar(body.value("session_id", ""));

          if (deleted.failed())
          {
            write_account_error(res, deleted.error());
            return;
          }

          if (!deleted.value().empty())
          {
            try { std::filesystem::remove(deleted.value()); } catch (...) {}
          }

          json_ok(res, vix::json::o("message", "Avatar removed."));
          return;
        }

        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      auto deleted = auth_service().delete_avatar(session_id);

      if (deleted.failed())
      {
        write_account_error(res, deleted.error());
        return;
      }

      if (!deleted.value().empty())
      {
        try { std::filesystem::remove(deleted.value()); } catch (...) {}
      }

      json_ok(res, vix::json::o("message", "Avatar removed.")); });

    app.get("/storage/users/{user_id}/{filename}", [](vix::Request &req, vix::Response &res)
            {
      const auto user_id = req.param("user_id");
      const auto filename = req.param("filename");

      if (!safe_path_segment(user_id) || !safe_path_segment(filename) || filename.rfind("avatar.", 0) != 0)
      {
        json_error(res, 404, "avatar_not_found", "Avatar was not found.");
        return;
      }

      const auto path = std::filesystem::path{"storage/users"} / user_id / filename;

      if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
      {
        json_error(res, 404, "avatar_not_found", "Avatar was not found.");
        return;
      }

      std::ifstream in(path, std::ios::binary);

      if (!in.good())
      {
        json_error(res, 404, "avatar_not_found", "Avatar was not found.");
        return;
      }

      std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      res.header("Content-Type", content_type_for_avatar_filename(filename));
      res.header("Cache-Control", "public, max-age=300");
      res.res.set_body(std::move(body)); });

    app.post("/api/auth/change_password", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto changed = auth_service().change_password(read_change_password_request(body));

      if (changed.failed())
      {
        write_account_error(res, changed.error());
        return;
      }

      json_ok(res, vix::json::o("message", "Password changed successfully.")); });
  }
} // namespace cloud::auth::controllers
