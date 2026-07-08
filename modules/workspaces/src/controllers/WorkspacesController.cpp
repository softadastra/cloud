/**
 *
 *  @file WorkspacesController.cpp
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
#include <workspaces/controllers/WorkspacesController.hpp>

#include <workspaces/dto/WorkspaceRequests.hpp>
#include <workspaces/services/WorkspaceService.hpp>
#include <workspaces/support/WorkspaceErrors.hpp>

#include <string>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::workspaces::controllers
{
  namespace
  {
    services::WorkspaceService &workspace_service()
    {
      static services::WorkspaceService service;
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
              "message", message));
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

    dto::CreateWorkspaceRequest read_create_workspace_request(
        const vix::json::Json &body)
    {
      return {
          body.value("name", ""),
          body.value("slug", ""),
          body.value("owner_user_id", ""),
      };
    }

    dto::UpdateWorkspaceRequest read_update_workspace_request(
        const vix::json::Json &body)
    {
      return {
          body.value("id", ""),
          body.value("name", ""),
          body.value("slug", ""),
      };
    }

    dto::WorkspaceLookupRequest read_workspace_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("id", ""),
      };
    }

    std::string read_owner_user_id(
        const vix::json::Json &body)
    {
      return body.value("owner_user_id", "");
    }

    vix::json::Json workspace_list_to_json(
        const std::vector<dto::WorkspaceResponse> &workspaces)
    {
      auto items = vix::json::a();

      for (const auto &workspace : workspaces)
      {
        items.push_back(workspace.to_json());
      }

      return items;
    }
  } // namespace

  void WorkspacesController::register_routes(vix::App &app)
  {
    app.get("/api/workspaces", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Workspaces module is available"); });

    app.post("/api/workspaces", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_create_workspace_request(body);
      auto created = workspace_service().create_workspace(request);

      if (created.failed())
      {
        support::write_workspace_error(res, created.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "workspace", created.value().to_json()))); });

    app.post("/api/workspaces/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_workspace_lookup_request(body);
      auto workspace = workspace_service().find_workspace(request);

      if (workspace.failed())
      {
        support::write_workspace_error(res, workspace.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "workspace", workspace.value().to_json())); });

    app.post("/api/workspaces/update", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_update_workspace_request(body);
      auto updated = workspace_service().update_workspace(request);

      if (updated.failed())
      {
        support::write_workspace_error(res, updated.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "workspace", updated.value().to_json())); });

    app.post("/api/workspaces/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto owner_user_id = read_owner_user_id(body);
      auto workspaces = workspace_service().list_workspaces_for_owner(owner_user_id);

      if (workspaces.failed())
      {
        support::write_workspace_error(res, workspaces.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "workspaces", workspace_list_to_json(workspaces.value()))); });
  }
} // namespace cloud::workspaces::controllers
