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

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
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
      if (filename.size() >= 4 && filename.rfind(".png") == filename.size() - 4)
      {
        return "image/png";
      }

      if (filename.size() >= 5 && filename.rfind(".webp") == filename.size() - 5)
      {
        return "image/webp";
      }

      return "image/jpeg";
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



    app.post("/api/workspaces/suspend", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto updated = workspace_service().set_status(body.value("id", ""), "suspended");

      if (updated.failed())
      {
        support::write_workspace_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o("workspace", updated.value().to_json())); });

    app.post("/api/workspaces/reactivate", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto updated = workspace_service().set_status(body.value("id", ""), "active");

      if (updated.failed())
      {
        support::write_workspace_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o("workspace", updated.value().to_json())); });

    app.post("/api/workspaces/delete", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto updated = workspace_service().set_status(body.value("id", ""), "deleted");

      if (updated.failed())
      {
        support::write_workspace_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o("workspace", updated.value().to_json())); });

    app.post("/api/workspaces/avatar/upload", [](vix::Request &req, vix::Response &res)
             {
      const auto workspace_id = req.has_query("workspace_id") ? req.query_value("workspace_id") : std::string{};

      if (workspace_id.empty())
      {
        json_error(res, 400, "workspace_required", "Workspace is required.");
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
        json_error(res, 400, "invalid_content_type", "Workspace avatar must be a JPG, PNG or WebP image.");
        return;
      }

      const auto &body = req.body();
      constexpr std::size_t max_avatar_size = 2u * 1024u * 1024u;

      if (body.size() > max_avatar_size)
      {
        json_error(res, 413, "avatar_too_large", "Workspace avatar image must be 2MB or smaller.");
        return;
      }

      if (body.empty())
      {
        json_error(res, 400, "avatar_upload_failed", "Workspace avatar image body is required.");
        return;
      }

      const std::filesystem::path storage_root{"storage/workspaces"};
      const auto avatar_dir = storage_root / workspace_id;
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
          json_error(res, 500, "avatar_upload_failed", "Could not write workspace avatar image.");
          return;
        }

        out.write(body.data(), static_cast<std::streamsize>(body.size()));
      }
      catch (...)
      {
        json_error(res, 500, "avatar_upload_failed", "Could not write workspace avatar image.");
        return;
      }

      const auto avatar_url = std::string{"/storage/workspaces/"} + workspace_id + "/" + filename;
      auto updated = workspace_service().update_avatar(workspace_id, avatar_url, avatar_path.string());

      if (updated.failed())
      {
        support::write_workspace_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o("avatar_url", avatar_url, "workspace", updated.value().to_json())); });

    app.post("/api/workspaces/avatar/delete", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto deleted = workspace_service().delete_avatar(body.value("id", ""));

      if (deleted.failed())
      {
        support::write_workspace_error(res, deleted.error());
        return;
      }

      if (!deleted.value().empty())
      {
        try { std::filesystem::remove(deleted.value()); } catch (...) {}
      }

      json_ok(res, vix::json::o("message", "Workspace avatar removed.")); });

    app.get("/storage/workspaces/{workspace_id}/{filename}", [](vix::Request &req, vix::Response &res)
            {
      const auto workspace_id = req.param("workspace_id");
      const auto filename = req.param("filename");

      if (!safe_path_segment(workspace_id) || !safe_path_segment(filename) || filename.rfind("avatar.", 0) != 0)
      {
        json_error(res, 404, "avatar_not_found", "Workspace avatar was not found.");
        return;
      }

      const auto path = std::filesystem::path{"storage/workspaces"} / workspace_id / filename;

      if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
      {
        json_error(res, 404, "avatar_not_found", "Workspace avatar was not found.");
        return;
      }

      std::ifstream in(path, std::ios::binary);

      if (!in.good())
      {
        json_error(res, 404, "avatar_not_found", "Workspace avatar was not found.");
        return;
      }

      std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      res.header("Content-Type", content_type_for_avatar_filename(filename));
      res.header("Cache-Control", "public, max-age=300");
      res.res.set_body(std::move(body)); });

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
