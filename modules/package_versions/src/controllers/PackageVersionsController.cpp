/**
 *
 *  @file PackageVersionsController.cpp
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
#include <package_versions/controllers/PackageVersionsController.hpp>

#include <package_versions/dto/PackageVersionRequests.hpp>
#include <package_versions/services/PackageVersionService.hpp>
#include <package_versions/support/PackageVersionErrors.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::package_versions::controllers
{
  namespace
  {
    services::PackageVersionService &package_version_service()
    {
      static services::PackageVersionService service;
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

    dto::PublishPackageVersionRequest read_publish_package_version_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("package_id", ""),
          body.value("published_by_user_id", ""),
          body.value("version", ""),
          body.value("archive_url", ""),
          body.value("checksum_sha256", ""),
          body.value("manifest_json", ""),
          body.value("size_bytes", static_cast<std::int64_t>(0)),
      };
    }

    dto::PackageVersionLookupRequest read_package_version_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("package_id", ""),
          body.value("version_id", ""),
      };
    }

    dto::PackageVersionByNumberRequest read_package_version_by_number_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("package_id", ""),
          body.value("version", ""),
      };
    }

    dto::ListPackageVersionsRequest read_list_package_versions_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("package_id", ""),
      };
    }

    vix::json::Json package_version_list_to_json(
        const std::vector<dto::PackageVersionResponse> &versions)
    {
      auto items = vix::json::a();

      for (const auto &version : versions)
      {
        items.push_back(version.to_json());
      }

      return items;
    }
  } // namespace

  void PackageVersionsController::register_routes(vix::App &app)
  {
    app.get("/api/package_versions", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Package versions module is available"); });

    app.post("/api/package_versions/publish", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_publish_package_version_request(body);
      auto published = package_version_service().publish_package_version(request);

      if (published.failed())
      {
        support::write_package_version_error(res, published.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "package_version", published.value().to_json()))); });

    app.post("/api/package_versions/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_list_package_versions_request(body);
      auto versions = package_version_service().list_package_versions(request);

      if (versions.failed())
      {
        support::write_package_version_error(res, versions.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "package_versions", package_version_list_to_json(versions.value()))); });

    app.post("/api/package_versions/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_package_version_lookup_request(body);
      auto version = package_version_service().find_package_version(request);

      if (version.failed())
      {
        support::write_package_version_error(res, version.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "package_version", version.value().to_json())); });

    app.post("/api/package_versions/resolve", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_package_version_by_number_request(body);
      auto version = package_version_service().find_package_version_by_number(request);

      if (version.failed())
      {
        support::write_package_version_error(res, version.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "package_version", version.value().to_json())); });
  }
} // namespace cloud::package_versions::controllers
