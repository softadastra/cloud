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

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <vix/config/Config.hpp>
#include <vix/db/Sha256.hpp>
#include <vix/db/db.hpp>
#include <vix/json.hpp>

namespace cloud::auth::middleware
{
  namespace
  {
    services::AuthService &auth_service()
    {
      static services::AuthService service;
      return service;
    }

    std::unique_ptr<vix::db::Database> make_database()
    {
      vix::config::Config cfg{".env"};
      return std::make_unique<vix::db::Database>(cfg);
    }

    bool starts_with(
        const std::string &value,
        const std::string &prefix)
    {
      return value.rfind(prefix, 0) == 0;
    }

    bool is_public_api_path(
        const std::string &path)
    {
      return path == "/api" ||
             path == "/api/health" ||
             path == "/api/ready" ||
             path == "/health" ||
             path == "/ready" ||
             starts_with(path, "/api/auth");
    }

    bool is_module_probe(
        const vix::Request &req)
    {
      return req.method() == "GET" &&
             (req.path() == "/api/workspaces" ||
              req.path() == "/api/members" ||
              req.path() == "/api/workspace_invites" ||
              req.path() == "/api/tokens" ||
              req.path() == "/api/projects" ||
              req.path() == "/api/packages" ||
              req.path() == "/api/package_versions" ||
              req.path() == "/api/lockfiles" ||
              req.path() == "/api/build_reports" ||
              req.path() == "/api/notifications" ||
              req.path() == "/api/activity" ||
              req.path() == "/api/feedback");
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

    std::string bearer_token(
        const vix::Request &req)
    {
      auto value = req.header("Authorization");

      if (value.empty())
      {
        value = req.header("authorization");
      }

      const std::string prefix = "Bearer ";

      if (starts_with(value, prefix))
      {
        return value.substr(prefix.size());
      }

      return {};
    }

    std::string session_id_from_request(
        const vix::Request &req)
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

      const auto bearer = bearer_token(req);

      if (!starts_with(bearer, "sdc_"))
      {
        return bearer;
      }

      return {};
    }

    std::string access_token_from_request(
        const vix::Request &req)
    {
      auto token = req.header("X-Access-Token");

      if (token.empty())
      {
        token = req.header("x-access-token");
      }

      if (!token.empty())
      {
        return token;
      }

      const auto bearer = bearer_token(req);

      if (starts_with(bearer, "sdc_"))
      {
        return bearer;
      }

      return {};
    }

    std::string json_value(
        const vix::Request &req,
        const std::string &key)
    {
      if (req.body().empty())
      {
        return {};
      }

      try
      {
        const auto &body = req.json();

        if (!body.is_object())
        {
          return {};
        }

        return body.value(key, "");
      }
      catch (...)
      {
        return {};
      }
    }

    std::string workspace_id_from_request(
        const vix::Request &req)
    {
      if (req.has_param("workspace_id"))
      {
        return req.param("workspace_id");
      }

      if (req.has_query("workspace_id"))
      {
        return req.query_value("workspace_id");
      }

      const auto workspace_id = json_value(req, "workspace_id");

      if (!workspace_id.empty())
      {
        return workspace_id;
      }

      if (starts_with(req.path(), "/api/workspaces/show") ||
          starts_with(req.path(), "/api/workspaces/update"))
      {
        return json_value(req, "id");
      }

      return {};
    }

    std::string project_id_from_request(
        const vix::Request &req)
    {
      if (req.has_param("project_id"))
      {
        return req.param("project_id");
      }

      if (req.has_query("project_id"))
      {
        return req.query_value("project_id");
      }

      auto project_id = json_value(req, "project_id");

      if (!project_id.empty())
      {
        return project_id;
      }

      if (starts_with(req.path(), "/api/projects/show") ||
          starts_with(req.path(), "/api/projects/update"))
      {
        return json_value(req, "id");
      }

      return {};
    }

    bool project_allowed_by_scope(
        const AuthContext &ctx,
        const std::string &project_id)
    {
      if (project_id.empty() || ctx.access_scope != "selected_projects")
      {
        return true;
      }

      return ctx.project_ids_json.find("\"" + project_id + "\"") != std::string::npos ||
             ctx.project_ids_json.find(project_id) != std::string::npos;
    }

    std::string actor_user_id_from_request(
        const vix::Request &req)
    {
      if (starts_with(req.path(), "/api/members"))
      {
        auto actor_user_id = json_value(req, "actor_user_id");
        if (!actor_user_id.empty())
        {
          return actor_user_id;
        }
        return json_value(req, "invited_by_user_id");
      }

      if (starts_with(req.path(), "/api/workspace_invites/create"))
      {
        return json_value(req, "invited_by_user_id");
      }

      if (starts_with(req.path(), "/api/workspace_invites/revoke"))
      {
        return json_value(req, "revoked_by_user_id");
      }

      if (starts_with(req.path(), "/api/workspace_invites/list_mine") ||
          starts_with(req.path(), "/api/workspace_invites/accept") ||
          starts_with(req.path(), "/api/workspace_invites/decline"))
      {
        return json_value(req, "user_id");
      }

      if (starts_with(req.path(), "/api/tokens"))
      {
        return json_value(req, "user_id");
      }

      if (starts_with(req.path(), "/api/notifications") ||
          starts_with(req.path(), "/api/activity") ||
          starts_with(req.path(), "/api/feedback"))
      {
        return json_value(req, "user_id");
      }

      for (const auto &key : {"owner_user_id", "uploaded_by_user_id", "submitted_by_user_id", "published_by_user_id"})
      {
        const auto value = json_value(req, key);

        if (!value.empty())
        {
          return value;
        }
      }

      return {};
    }

    bool action_is_read(
        const vix::Request &req)
    {
      return req.method() == "GET" ||
             req.path().find("/list") != std::string::npos ||
             req.path().find("/show") != std::string::npos ||
             req.path().find("/resolve") != std::string::npos ||
             req.path().find("/download") != std::string::npos;
    }

    bool action_requires_admin(
        const vix::Request &req)
    {
      if (starts_with(req.path(), "/api/tokens"))
      {
        return true;
      }

      if (starts_with(req.path(), "/api/feedback/list_workspace") ||
          starts_with(req.path(), "/api/feedback/update_status"))
      {
        return true;
      }

      if (starts_with(req.path(), "/api/workspace_invites/create") ||
          starts_with(req.path(), "/api/workspace_invites/list") ||
          starts_with(req.path(), "/api/workspace_invites/revoke"))
      {
        return true;
      }

      return starts_with(req.path(), "/api/members") && !action_is_read(req);
    }

    bool role_allows(
        const std::string &role,
        const vix::Request &req)
    {
      if (role == "owner")
      {
        return true;
      }

      if (action_requires_admin(req))
      {
        return role == "admin";
      }

      if (action_is_read(req))
      {
        return role == "admin" || role == "member" || role == "viewer";
      }

      return role == "admin" || role == "member";
    }

    bool workspace_owner_or_member_context(
        vix::db::Database &db,
        const std::string &workspace_id,
        const std::string &user_id,
        AuthContext &ctx)
    {
      auto owner = db.query(
          "SELECT id FROM workspaces WHERE id = ? AND owner_user_id = ? LIMIT 1",
          workspace_id,
          user_id);

      if (owner->next())
      {
        ctx.role = "owner";
        ctx.status = "active";
        ctx.access_scope = "entire_workspace";
        ctx.project_ids_json = "";
        return true;
      }

      auto member = db.query(
          "SELECT role, status, COALESCE(access_scope, 'entire_workspace'), COALESCE(project_ids_json, '') "
          "FROM workspace_members WHERE workspace_id = ? AND user_id = ? AND active = 1 AND status = 'active' LIMIT 1",
          workspace_id,
          user_id);

      if (!member->next())
      {
        return false;
      }

      ctx.role = member->row().getString(0);
      ctx.status = member->row().getString(1);
      ctx.access_scope = member->row().getString(2);
      ctx.project_ids_json = member->row().getString(3);
      return true;
    }

    bool authenticate_cli_token(
        const std::string &token,
        const std::string &workspace_id,
        AuthContext &ctx)
    {
      if (token.empty() || workspace_id.empty())
      {
        return false;
      }

      auto db = make_database();
      const auto token_hash = vix::db::sha256_hex(token);
      auto rows = db->query(
          "SELECT user_id, workspace_id FROM access_tokens "
          "WHERE token_hash = ? AND workspace_id = ? AND revoked_at IS NULL AND expires_at > strftime('%s','now') "
          "LIMIT 1",
          token_hash,
          workspace_id);

      if (!rows->next())
      {
        return false;
      }

      ctx.user_id = rows->row().getString(0);
      ctx.workspace_id = rows->row().getString(1);
      ctx.role = "member";
      ctx.status = "active";
      ctx.access_scope = "entire_workspace";
      ctx.cli_token = true;

      db->exec(
          "UPDATE access_tokens SET last_used_at = strftime('%s','now'), updated_at = strftime('%s','now') WHERE token_hash = ?",
          token_hash);

      return true;
    }

    bool authenticate_session_context(
        const std::string &session_id,
        AuthContext &ctx)
    {
      if (session_id.empty())
      {
        return false;
      }

      auto session = authenticate_session(session_id);

      if (session.failed())
      {
        return false;
      }

      ctx.user_id = session.value().user_id();
      ctx.cli_token = false;
      return true;
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

  bool require_api_auth_and_workspace_permission(
      vix::Request &req,
      vix::Response &res)
  {
    if (is_public_api_path(req.path()) || is_module_probe(req))
    {
      return true;
    }

    AuthContext ctx;
    const auto workspace_id = workspace_id_from_request(req);

    const auto access_token = access_token_from_request(req);

    if (!access_token.empty())
    {
      if (!authenticate_cli_token(access_token, workspace_id, ctx))
      {
        json_error(res, 401, "unauthorized", "Invalid or expired access token.");
        return false;
      }
    }
    else if (!authenticate_session_context(session_id_from_request(req), ctx))
    {
      json_error(res, 401, "unauthorized", "Authentication is required.");
      return false;
    }

    const auto request_user_id = actor_user_id_from_request(req);

    if (!request_user_id.empty() && request_user_id != ctx.user_id)
    {
      json_error(res, 403, "forbidden", "Authenticated user does not match request user.");
      return false;
    }

    if (starts_with(req.path(), "/api/workspaces") && workspace_id.empty())
    {
      req.set_state<AuthContext>(ctx);
      return true;
    }

    if ((starts_with(req.path(), "/api/workspace_invites/list_mine") ||
         starts_with(req.path(), "/api/workspace_invites/accept") ||
         starts_with(req.path(), "/api/workspace_invites/decline") ||
         starts_with(req.path(), "/api/notifications") ||
         starts_with(req.path(), "/api/feedback/create") ||
         starts_with(req.path(), "/api/feedback/list_mine")) &&
        workspace_id.empty())
    {
      ctx.role = "member";
      ctx.status = "active";
      ctx.access_scope = "entire_workspace";
      req.set_state<AuthContext>(ctx);
      return true;
    }

    if (workspace_id.empty())
    {
      json_error(res, 400, "missing_workspace", "Workspace is required for this route.");
      return false;
    }

    if (!ctx.cli_token)
    {
      auto db = make_database();
      if (!workspace_owner_or_member_context(*db, workspace_id, ctx.user_id, ctx))
      {
        json_error(res, 403, "forbidden", "User does not belong to this workspace.");
        return false;
      }

      ctx.workspace_id = workspace_id;
    }

    if (!role_allows(ctx.role, req))
    {
      json_error(res, 403, "forbidden", "User role does not allow this action.");
      return false;
    }

    const auto project_id = project_id_from_request(req);
    if (!project_allowed_by_scope(ctx, project_id))
    {
      json_error(res, 403, "forbidden", "User does not have access to this project.");
      return false;
    }

    if (ctx.access_scope == "selected_projects" &&
        starts_with(req.path(), "/api/projects") &&
        !action_is_read(req))
    {
      json_error(res, 403, "forbidden", "Selected project access cannot create or modify projects.");
      return false;
    }

    req.set_state<AuthContext>(ctx);
    return true;
  }
} // namespace cloud::auth::middleware
