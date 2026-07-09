/**
 *
 *  @file ProjectsController.cpp
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
#include <projects/controllers/ProjectsController.hpp>

#include <projects/dto/ProjectRequests.hpp>
#include <projects/services/ProjectService.hpp>
#include <projects/support/ProjectErrors.hpp>
#include <auth/middleware/AuthMiddleware.hpp>

#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::projects::controllers
{
  namespace
  {
    services::ProjectService &project_service()
    {
      static services::ProjectService service;
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

    dto::CreateProjectRequest read_create_project_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("owner_user_id", ""),
          body.value("name", ""),
          body.value("slug", ""),
          body.value("description", ""),
          body.value("repository_url", ""),
          body.value("default_branch", ""),
      };
    }

    dto::UpdateProjectRequest read_update_project_request(
        const vix::json::Json &body)
    {
      return {
          body.value("id", ""),
          body.value("workspace_id", ""),
          body.value("name", ""),
          body.value("slug", ""),
          body.value("description", ""),
          body.value("repository_url", ""),
          body.value("default_branch", ""),
      };
    }

    dto::ProjectLookupRequest read_project_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("id", ""),
          body.value("workspace_id", ""),
      };
    }

    dto::ListProjectsRequest read_list_projects_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          "",
          "",
      };
    }

    vix::json::Json project_list_to_json(
        const std::vector<dto::ProjectResponse> &projects)
    {
      auto items = vix::json::a();

      for (const auto &project : projects)
      {
        items.push_back(project.to_json());
      }

      return items;
    }
  } // namespace

  void ProjectsController::register_routes(vix::App &app)
  {
    app.get("/api/projects", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Projects module is available"); });

    app.post("/api/projects", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_create_project_request(body);
      auto created = project_service().create_project(request);

      if (created.failed())
      {
        support::write_project_error(res, created.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "project", created.value().to_json()))); });

    app.post("/api/projects/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto request = read_list_projects_request(body);
      if (auto *ctx = req.try_state<cloud::auth::middleware::AuthContext>())
      {
        request.access_scope = ctx->access_scope;
        request.project_ids_json = ctx->project_ids_json;
      }
      auto projects = project_service().list_projects(request);

      if (projects.failed())
      {
        support::write_project_error(res, projects.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "projects", project_list_to_json(projects.value()))); });

    app.post("/api/projects/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_project_lookup_request(body);
      auto project = project_service().find_project(request);

      if (project.failed())
      {
        support::write_project_error(res, project.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "project", project.value().to_json())); });

    app.post("/api/projects/update", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_update_project_request(body);
      auto updated = project_service().update_project(request);

      if (updated.failed())
      {
        support::write_project_error(res, updated.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "project", updated.value().to_json())); });
  }
} // namespace cloud::projects::controllers
