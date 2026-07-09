/**
 *
 *  @file PackagesController.cpp
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
#include <packages/controllers/PackagesController.hpp>

#include <packages/dto/PackageRequests.hpp>
#include <packages/services/PackageService.hpp>
#include <packages/support/PackageErrors.hpp>

#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::packages::controllers
{
  namespace
  {
    services::PackageService &package_service()
    {
      static services::PackageService service;
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

    dto::CreatePackageRequest read_create_package_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("owner_user_id", ""),
          body.value("name", ""),
          body.value("description", ""),
          body.value("repository_url", ""),
          body.value("visibility", ""),
      };
    }

    dto::UpdatePackageRequest read_update_package_request(
        const vix::json::Json &body)
    {
      return {
          body.value("id", ""),
          body.value("workspace_id", ""),
          body.value("name", ""),
          body.value("description", ""),
          body.value("repository_url", ""),
          body.value("visibility", ""),
      };
    }

    dto::PackageLookupRequest read_package_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("id", ""),
          body.value("workspace_id", ""),
      };
    }

    dto::ListPackagesRequest read_list_packages_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
      };
    }

    vix::json::Json package_list_to_json(
        const std::vector<dto::PackageResponse> &packages)
    {
      auto items = vix::json::a();

      for (const auto &package : packages)
      {
        items.push_back(package.to_json());
      }

      return items;
    }
  } // namespace

  void PackagesController::register_routes(vix::App &app)
  {
    app.get("/api/packages", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Packages module is available"); });

    app.post("/api/packages", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_create_package_request(body);
      auto created = package_service().create_package(request);

      if (created.failed())
      {
        support::write_package_error(res, created.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "package", created.value().to_json()))); });

    app.post("/api/packages/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_list_packages_request(body);
      auto packages = package_service().list_packages(request);

      if (packages.failed())
      {
        support::write_package_error(res, packages.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "packages", package_list_to_json(packages.value()))); });

    app.post("/api/packages/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_package_lookup_request(body);
      auto package = package_service().find_package(request);

      if (package.failed())
      {
        support::write_package_error(res, package.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "package", package.value().to_json())); });

    app.post("/api/packages/update", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_update_package_request(body);
      auto updated = package_service().update_package(request);

      if (updated.failed())
      {
        support::write_package_error(res, updated.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "package", updated.value().to_json())); });
  }
} // namespace cloud::packages::controllers
