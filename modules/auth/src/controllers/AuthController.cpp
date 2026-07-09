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
#include <auth/support/AuthErrors.hpp>

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

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "user", vix::json::o(
                      "id", user.id(),
                      "name", request.name,
                      "email", user.email(),
                      "email_verified", user.email_verified(),
                      "active", user.active())))); });

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

      json_ok(
          res,
          vix::json::o(
              "user", vix::json::o(
                  "id", result.user.id(),
                  "name", "",
                  "email", result.user.email(),
                  "email_verified", result.user.email_verified(),
                  "active", result.user.active()),
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

      json_ok(
          res,
          vix::json::o(
              "session", vix::json::o(
                  "id", session.value().id(),
                  "user_id", session.value().user_id(),
                  "expires_at", session.value().expires_at(),
                  "last_seen_at", session.value().last_seen_at(),
                  "revoked", session.value().revoked()))); });
  }
} // namespace cloud::auth::controllers
