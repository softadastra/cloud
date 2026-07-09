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
#include <filesystem>
#include <fstream>
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


    bool safe_path_segment(
        const std::string &value)
    {
      if (value.empty() || value == "." || value == "..")
      {
        return false;
      }

      for (const char ch : value)
      {
        if (ch == '/' || ch == '\\')
        {
          return false;
        }
      }

      return true;
    }

    std::filesystem::path package_archive_dir(
        const std::string &workspace_id,
        const std::string &package_id,
        const std::string &version)
    {
      return std::filesystem::path{"storage"} /
             "packages" /
             workspace_id /
             package_id /
             version;
    }

    bool write_binary_file(
        const std::filesystem::path &path,
        const std::string &body)
    {
      std::filesystem::create_directories(path.parent_path());

      std::ofstream out(path, std::ios::binary | std::ios::trunc);

      if (!out)
      {
        return false;
      }

      out.write(body.data(), static_cast<std::streamsize>(body.size()));
      return static_cast<bool>(out);
    }

    bool write_text_file(
        const std::filesystem::path &path,
        const std::string &body)
    {
      std::filesystem::create_directories(path.parent_path());

      std::ofstream out(path, std::ios::binary | std::ios::trunc);

      if (!out)
      {
        return false;
      }

      out << body;
      return static_cast<bool>(out);
    }

    bool read_binary_file(
        const std::filesystem::path &path,
        std::string &body)
    {
      std::ifstream in(path, std::ios::binary);

      if (!in)
      {
        return false;
      }

      in.seekg(0, std::ios::end);
      const auto size = in.tellg();

      if (size < 0)
      {
        return false;
      }

      in.seekg(0, std::ios::beg);
      body.resize(static_cast<std::size_t>(size));

      if (size > 0)
      {
        in.read(body.data(), size);
      }

      return in.good() || in.eof();
    }

    bool archive_path_allowed(
        const std::filesystem::path &path)
    {
      const auto normalized = path.lexically_normal().generic_string();
      return normalized.rfind("storage/packages/", 0) == 0 &&
             normalized.find("..") == std::string::npos &&
             path.filename() == "package.tar.gz";
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



    app.post("/api/package_versions/upload/{workspace_id}/{package_id}/{version}", [](vix::Request &req, vix::Response &res)
             {
      const auto workspace_id = req.param("workspace_id");
      const auto package_id = req.param("package_id");
      const auto version = req.param("version");

      if (!safe_path_segment(workspace_id) ||
          !safe_path_segment(package_id) ||
          !safe_path_segment(version))
      {
        json_error(
            res,
            400,
            "invalid_path",
            "Package archive path is invalid.");
        return;
      }

      const auto &archive_body = req.body();

      if (archive_body.empty())
      {
        json_error(
            res,
            400,
            "missing_archive",
            "Package archive body is required.");
        return;
      }

      const auto published_by_user_id = req.query_value("published_by_user_id");
      const auto checksum_sha256 = req.query_value("checksum_sha256");
      const auto manifest_json = req.query_value("manifest_json", "{}");

      const auto archive_dir = package_archive_dir(
          workspace_id,
          package_id,
          version);
      const auto archive_path = archive_dir / "package.tar.gz";
      const auto metadata_path = archive_dir / "metadata.json";

      if (!archive_path_allowed(archive_path))
      {
        json_error(
            res,
            400,
            "invalid_path",
            "Package archive path is invalid.");
        return;
      }

      if (std::filesystem::exists(archive_path))
      {
        json_error(
            res,
            409,
            "package_version_already_exists",
            "Package archive already exists.");
        return;
      }

      if (!write_binary_file(archive_path, archive_body))
      {
        json_error(
            res,
            500,
            "archive_write_failed",
            "Could not write package archive.");
        return;
      }

      const auto archive_url = archive_path.lexically_normal().generic_string();

      dto::PublishPackageVersionRequest request;
      request.workspace_id = workspace_id;
      request.package_id = package_id;
      request.published_by_user_id = published_by_user_id;
      request.version = version;
      request.archive_url = archive_url;
      request.checksum_sha256 = checksum_sha256;
      request.manifest_json = manifest_json;
      request.size_bytes = static_cast<std::int64_t>(archive_body.size());

      auto published = package_version_service().publish_package_version(request);

      if (published.failed())
      {
        std::error_code ignored;
        std::filesystem::remove(archive_path, ignored);
        support::write_package_version_error(res, published.error());
        return;
      }

      const auto metadata = vix::json::o(
          "workspace_id", workspace_id,
          "package_id", package_id,
          "version", version,
          "archive", "package.tar.gz",
          "checksum_sha256", checksum_sha256,
          "size_bytes", static_cast<std::int64_t>(archive_body.size()),
          "manifest_json", manifest_json);
      (void)write_text_file(metadata_path, metadata.dump());

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "package_version", published.value().to_json()))); });

    app.get("/api/package_versions/download/{workspace_id}/{package_id}/{version}", [](vix::Request &req, vix::Response &res)
            {
      const auto workspace_id = req.param("workspace_id");
      const auto package_id = req.param("package_id");
      const auto version = req.param("version");

      if (!safe_path_segment(workspace_id) ||
          !safe_path_segment(package_id) ||
          !safe_path_segment(version))
      {
        json_error(
            res,
            400,
            "invalid_path",
            "Package archive path is invalid.");
        return;
      }

      dto::PackageVersionByNumberRequest request;
      request.workspace_id = workspace_id;
      request.package_id = package_id;
      request.version = version;

      auto resolved = package_version_service().find_package_version_by_number(request);

      if (resolved.failed())
      {
        support::write_package_version_error(res, resolved.error());
        return;
      }

      const std::filesystem::path archive_path{resolved.value().archive_url};

      if (!archive_path_allowed(archive_path) ||
          !std::filesystem::exists(archive_path) ||
          !std::filesystem::is_regular_file(archive_path))
      {
        json_error(
            res,
            404,
            "archive_not_found",
            "Package archive not found.");
        return;
      }

      std::string body;

      if (!read_binary_file(archive_path, body))
      {
        json_error(
            res,
            500,
            "archive_read_failed",
            "Could not read package archive.");
        return;
      }

      res.status(200)
          .header("Content-Type", "application/gzip")
          .header("Content-Disposition", "attachment; filename=\"package.tar.gz\"");
      res.res.set_body(std::move(body)); });

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
