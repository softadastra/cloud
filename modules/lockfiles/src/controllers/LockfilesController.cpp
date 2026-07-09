/**
 *
 *  @file LockfilesController.cpp
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
#include <lockfiles/controllers/LockfilesController.hpp>

#include <lockfiles/dto/LockfileRequests.hpp>
#include <lockfiles/services/LockfileService.hpp>
#include <lockfiles/support/LockfileErrors.hpp>

#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::lockfiles::controllers
{
  namespace
  {
    services::LockfileService &lockfile_service()
    {
      static services::LockfileService service;
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

    dto::UploadLockfileRequest read_upload_lockfile_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("project_id", ""),
          body.value("uploaded_by_user_id", ""),
          body.value("lockfile_json", ""),
          body.value("checksum_sha256", ""),
          body.value("source", ""),
      };
    }

    dto::LockfileLookupRequest read_lockfile_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("project_id", ""),
          body.value("lockfile_id", ""),
      };
    }

    dto::ListLockfilesRequest read_list_lockfiles_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("project_id", ""),
      };
    }

    vix::json::Json lockfile_list_to_json(
        const std::vector<dto::LockfileResponse> &lockfiles)
    {
      auto items = vix::json::a();

      for (const auto &lockfile : lockfiles)
      {
        items.push_back(lockfile.to_json());
      }

      return items;
    }
  } // namespace

  void LockfilesController::register_routes(vix::App &app)
  {
    app.get("/api/lockfiles", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Lockfiles module is available"); });

    app.post("/api/lockfiles/upload", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_upload_lockfile_request(body);
      auto uploaded = lockfile_service().upload_lockfile(request);

      if (uploaded.failed())
      {
        support::write_lockfile_error(res, uploaded.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "lockfile", uploaded.value().to_json()))); });

    app.post("/api/lockfiles/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_list_lockfiles_request(body);
      auto lockfiles = lockfile_service().list_lockfiles(request);

      if (lockfiles.failed())
      {
        support::write_lockfile_error(res, lockfiles.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "lockfiles", lockfile_list_to_json(lockfiles.value()))); });

    app.post("/api/lockfiles/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_lockfile_lookup_request(body);
      auto lockfile = lockfile_service().find_lockfile(request);

      if (lockfile.failed())
      {
        support::write_lockfile_error(res, lockfile.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "lockfile", lockfile.value().to_json())); });
  }
} // namespace cloud::lockfiles::controllers
