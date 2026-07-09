/**
 *
 *  @file TokensController.cpp
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
#include <tokens/controllers/TokensController.hpp>

#include <tokens/dto/TokenRequests.hpp>
#include <tokens/services/TokenService.hpp>
#include <tokens/support/TokenErrors.hpp>

#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::tokens::controllers
{
  namespace
  {
    services::TokenService &token_service()
    {
      static services::TokenService service;
      return service;
    }

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

    std::vector<std::string> read_scopes(
        const vix::json::Json &body)
    {
      std::vector<std::string> scopes;

      if (!body.contains("scopes") || !body["scopes"].is_array())
      {
        return scopes;
      }

      for (const auto &scope : body["scopes"])
      {
        if (scope.is_string())
        {
          scopes.push_back(scope.get<std::string>());
        }
      }

      return scopes;
    }

    dto::CreateTokenRequest read_create_token_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("user_id", ""),
          body.value("name", ""),
          read_scopes(body),
      };
    }

    dto::RevokeTokenRequest read_revoke_token_request(
        const vix::json::Json &body)
    {
      return {
          body.value("token_id", ""),
          body.value("workspace_id", ""),
          body.value("user_id", ""),
      };
    }

    dto::ListTokensRequest read_list_tokens_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("user_id", ""),
      };
    }

    dto::TokenLookupRequest read_token_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("token_id", ""),
          body.value("workspace_id", ""),
          body.value("user_id", ""),
      };
    }

    vix::json::Json token_list_to_json(
        const std::vector<dto::TokenResponse> &tokens)
    {
      auto items = vix::json::a();

      for (const auto &token : tokens)
      {
        items.push_back(token.to_json());
      }

      return items;
    }
  } // namespace

  void TokensController::register_routes(vix::App &app)
  {
    app.get("/api/tokens", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Tokens module is available"); });

    app.post("/api/tokens/create", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_create_token_request(body);
      auto created = token_service().create_token(request);

      if (created.failed())
      {
        support::write_token_error(res, created.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "token", created.value().to_json()))); });

    app.post("/api/tokens/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_list_tokens_request(body);
      auto tokens = token_service().list_tokens(request);

      if (tokens.failed())
      {
        support::write_token_error(res, tokens.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "tokens", token_list_to_json(tokens.value()))); });

    app.post("/api/tokens/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_token_lookup_request(body);
      auto token = token_service().find_token(request);

      if (token.failed())
      {
        support::write_token_error(res, token.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "token", token.value().to_json())); });

    app.post("/api/tokens/revoke", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_revoke_token_request(body);
      auto revoked = token_service().revoke_token(request);

      if (revoked.failed())
      {
        support::write_token_error(res, revoked.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "token", revoked.value().to_json())); });
  }
} // namespace cloud::tokens::controllers
