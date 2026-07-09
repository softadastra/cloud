/**
 *
 *  @file PackageVersionService.cpp
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
#include <package_versions/services/PackageVersionService.hpp>
#include <notifications/services/NotificationService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::package_versions::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_package_version_id()
    {
      static std::atomic<std::uint64_t> counter{0};
      std::ostringstream stream;
      stream << "package_version_" << now_timestamp() << "_" << counter.fetch_add(1);
      return stream.str();
    }

    bool valid_version(const std::string &version)
    {
      if (version.size() < 1 || version.size() > 80)
      {
        return false;
      }
      for (const char ch : version)
      {
        const auto c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || ch == '.' || ch == '-' || ch == '+')
        {
          continue;
        }
        return false;
      }
      return true;
    }

    bool valid_checksum_sha256(const std::string &checksum)
    {
      if (checksum.size() != 64)
      {
        return false;
      }
      for (const char ch : checksum)
      {
        const auto c = static_cast<unsigned char>(ch);
        if (!std::isxdigit(c))
        {
          return false;
        }
      }
      return true;
    }

    bool valid_manifest(const std::string &manifest_json)
    {
      return !manifest_json.empty();
    }

    std::string version_key(const std::string &workspace_id, const std::string &package_id, const std::string &version)
    {
      return workspace_id + "::" + package_id + "::" + version;
    }
  } // namespace

  class PackageVersionService::Impl
  {
  public:
    Impl()
    {
      const char *use_db = std::getenv("SOFTADASTRA_CLOUD_USE_DB");
      if (use_db != nullptr && std::string(use_db) == "1")
      {
        vix::config::Config cfg{".env"};
        db = std::make_unique<vix::db::Database>(cfg);
      }
    }

    bool persistent() const { return db != nullptr; }

    bool package_exists(const std::string &workspace_id, const std::string &package_id) const
    {
      auto rows = db->query("SELECT id FROM packages WHERE id = ? AND workspace_id = ? LIMIT 1", package_id, workspace_id);
      return rows->next();
    }

    bool version_exists(const std::string &package_id, const std::string &version) const
    {
      auto rows = db->query("SELECT id FROM package_versions WHERE package_id = ? AND version = ? LIMIT 1", package_id, version);
      return rows->next();
    }

    dto::PackageVersionResponse row_to_version(const vix::db::ResultRow &row) const
    {
      dto::PackageVersionResponse version;
      version.id = row.getString(0);
      version.workspace_id = row.getString(1);
      version.package_id = row.getString(2);
      version.published_by_user_id = row.getString(3);
      version.version = row.getString(4);
      version.archive_url = row.getString(5);
      version.checksum_sha256 = row.getString(6);
      version.manifest_json = row.getString(7);
      version.status = row.getString(8);
      version.size_bytes = row.getInt64(9);
      version.created_at = row.getInt64(10);
      version.updated_at = row.getInt64(11);
      return version;
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::PackageVersionResponse> versions_by_id;
    std::unordered_map<std::string, std::string> version_id_by_package_version;
  };

  PackageVersionService::PackageVersionService() : impl_(std::make_unique<Impl>()) {}
  PackageVersionService::~PackageVersionService() = default;
  PackageVersionService::PackageVersionService(PackageVersionService &&) noexcept = default;
  PackageVersionService &PackageVersionService::operator=(PackageVersionService &&) noexcept = default;

  PackageVersionResult<dto::PackageVersionResponse> PackageVersionService::publish_package_version(const dto::PublishPackageVersionRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.package_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage, "Package is required."});
    }
    if (request.published_by_user_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPublisher, "Publisher is required."});
    }
    if (!valid_version(request.version))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidVersion, "Package version must contain only letters, numbers, dots, dashes or plus signs."});
    }
    if (request.archive_url.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingArchive, "Package archive is required."});
    }
    if (!valid_checksum_sha256(request.checksum_sha256))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidChecksum, "Package checksum must be a valid SHA-256 value."});
    }
    if (!valid_manifest(request.manifest_json))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidManifest, "Package manifest is required."});
    }
    if (request.size_bytes <= 0)
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidSize, "Package archive size must be greater than zero."});
    }

    if (impl_->persistent())
    {
      try
      {
        if (!impl_->package_exists(request.workspace_id, request.package_id))
        {
          return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage, "Package not found."});
        }
        if (impl_->version_exists(request.package_id, request.version))
        {
          return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionAlreadyExists, "Package version already exists."});
        }

        const auto timestamp = now_timestamp();
        dto::PackageVersionResponse version;
        version.id = make_package_version_id();
        version.workspace_id = request.workspace_id;
        version.package_id = request.package_id;
        version.published_by_user_id = request.published_by_user_id;
        version.version = request.version;
        version.archive_url = request.archive_url;
        version.checksum_sha256 = request.checksum_sha256;
        version.manifest_json = request.manifest_json;
        version.status = "published";
        version.size_bytes = request.size_bytes;
        version.created_at = timestamp;
        version.updated_at = timestamp;

        impl_->db->exec(
            "INSERT INTO package_versions (id, workspace_id, package_id, published_by_user_id, version, archive_url, checksum_sha256, manifest_json, status, size_bytes, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            version.id, version.workspace_id, version.package_id, version.published_by_user_id, version.version, version.archive_url,
            version.checksum_sha256, version.manifest_json, version.status, version.size_bytes, version.created_at, version.updated_at);

        cloud::notifications::services::NotificationService notifications;
        cloud::notifications::dto::CreateNotificationRequest note;
        note.workspace_id = version.workspace_id;
        note.actor_user_id = version.published_by_user_id;
        note.type = "package_version_published";
        note.title = "Package version published";
        note.message = "A package version was published: " + version.version + ".";
        note.data_json = "{}";
        notifications.create_for_workspace_members(note);

        return PackageVersionResult<dto::PackageVersionResponse>::success(version);
      }
      catch (const std::exception &error)
      {
        (void)error;
        return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionAlreadyExists, "Package version already exists."});
      }
    }

    const auto key = version_key(request.workspace_id, request.package_id, request.version);
    if (impl_->version_id_by_package_version.find(key) != impl_->version_id_by_package_version.end())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionAlreadyExists, "Package version already exists."});
    }

    const auto timestamp = now_timestamp();
    dto::PackageVersionResponse version;
    version.id = make_package_version_id();
    version.workspace_id = request.workspace_id;
    version.package_id = request.package_id;
    version.published_by_user_id = request.published_by_user_id;
    version.version = request.version;
    version.archive_url = request.archive_url;
    version.checksum_sha256 = request.checksum_sha256;
    version.manifest_json = request.manifest_json;
    version.status = "published";
    version.size_bytes = request.size_bytes;
    version.created_at = timestamp;
    version.updated_at = timestamp;

    impl_->version_id_by_package_version[key] = version.id;
    impl_->versions_by_id[version.id] = version;
    return PackageVersionResult<dto::PackageVersionResponse>::success(version);
  }

  PackageVersionResult<dto::PackageVersionResponse> PackageVersionService::find_package_version(const dto::PackageVersionLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.package_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage, "Package is required."});
    }

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, package_id, published_by_user_id, version, archive_url, checksum_sha256, manifest_json, status, size_bytes, created_at, updated_at FROM package_versions WHERE id = ? AND workspace_id = ? AND package_id = ? LIMIT 1",
          request.version_id, request.workspace_id, request.package_id);
      if (!rows->next())
      {
        return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound, "Package version not found."});
      }
      return PackageVersionResult<dto::PackageVersionResponse>::success(impl_->row_to_version(rows->row()));
    }

    auto item = impl_->versions_by_id.find(request.version_id);
    if (item == impl_->versions_by_id.end() || item->second.workspace_id != request.workspace_id || item->second.package_id != request.package_id)
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound, "Package version not found."});
    }
    return PackageVersionResult<dto::PackageVersionResponse>::success(item->second);
  }

  PackageVersionResult<dto::PackageVersionResponse> PackageVersionService::find_package_version_by_number(const dto::PackageVersionByNumberRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.package_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage, "Package is required."});
    }
    if (!valid_version(request.version))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidVersion, "Package version is invalid."});
    }

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, package_id, published_by_user_id, version, archive_url, checksum_sha256, manifest_json, status, size_bytes, created_at, updated_at FROM package_versions WHERE workspace_id = ? AND package_id = ? AND version = ? LIMIT 1",
          request.workspace_id, request.package_id, request.version);
      if (!rows->next())
      {
        return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound, "Package version not found."});
      }
      return PackageVersionResult<dto::PackageVersionResponse>::success(impl_->row_to_version(rows->row()));
    }

    const auto key = version_key(request.workspace_id, request.package_id, request.version);
    auto version_id = impl_->version_id_by_package_version.find(key);
    if (version_id == impl_->version_id_by_package_version.end())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound, "Package version not found."});
    }
    auto version = impl_->versions_by_id.find(version_id->second);
    if (version == impl_->versions_by_id.end())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound, "Package version not found."});
    }
    return PackageVersionResult<dto::PackageVersionResponse>::success(version->second);
  }

  PackageVersionResult<std::vector<dto::PackageVersionResponse>> PackageVersionService::list_package_versions(const dto::ListPackageVersionsRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::failure({support::PackageVersionErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.package_id.empty())
    {
      return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::failure({support::PackageVersionErrorCode::MissingPackage, "Package is required."});
    }

    std::vector<dto::PackageVersionResponse> versions;
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, package_id, published_by_user_id, version, archive_url, checksum_sha256, manifest_json, status, size_bytes, created_at, updated_at FROM package_versions WHERE workspace_id = ? AND package_id = ? ORDER BY created_at",
          request.workspace_id, request.package_id);
      while (rows->next())
      {
        versions.push_back(impl_->row_to_version(rows->row()));
      }
      return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::success(versions);
    }

    for (const auto &entry : impl_->versions_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id && entry.second.package_id == request.package_id)
      {
        versions.push_back(entry.second);
      }
    }
    std::sort(versions.begin(), versions.end(), [](const auto &left, const auto &right) { return left.created_at < right.created_at; });
    return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::success(versions);
  }
} // namespace cloud::package_versions::services
