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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>

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
      stream << "package_version_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    bool valid_version(
        const std::string &version)
    {
      if (version.size() < 1 || version.size() > 80)
      {
        return false;
      }

      for (const char ch : version)
      {
        const auto c = static_cast<unsigned char>(ch);

        if (std::isalnum(c) ||
            ch == '.' ||
            ch == '-' ||
            ch == '+')
        {
          continue;
        }

        return false;
      }

      return true;
    }

    bool valid_checksum_sha256(
        const std::string &checksum)
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

    bool valid_manifest(
        const std::string &manifest_json)
    {
      return !manifest_json.empty();
    }

    std::string version_key(
        const std::string &workspace_id,
        const std::string &package_id,
        const std::string &version)
    {
      return workspace_id + "::" + package_id + "::" + version;
    }
  } // namespace

  class PackageVersionService::Impl
  {
  public:
    std::unordered_map<std::string, dto::PackageVersionResponse> versions_by_id;
    std::unordered_map<std::string, std::string> version_id_by_package_version;
  };

  PackageVersionService::PackageVersionService()
      : impl_(std::make_unique<Impl>())
  {
  }

  PackageVersionService::~PackageVersionService() = default;

  PackageVersionService::PackageVersionService(PackageVersionService &&) noexcept = default;

  PackageVersionService &PackageVersionService::operator=(PackageVersionService &&) noexcept = default;

  PackageVersionResult<dto::PackageVersionResponse> PackageVersionService::publish_package_version(
      const dto::PublishPackageVersionRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingWorkspace,
                                                                         "Workspace is required."});
    }

    if (request.package_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage,
                                                                         "Package is required."});
    }

    if (request.published_by_user_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPublisher,
                                                                         "Publisher is required."});
    }

    if (!valid_version(request.version))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidVersion,
                                                                         "Package version must contain only letters, numbers, dots, dashes or plus signs."});
    }

    if (request.archive_url.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingArchive,
                                                                         "Package archive is required."});
    }

    if (!valid_checksum_sha256(request.checksum_sha256))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidChecksum,
                                                                         "Package checksum must be a valid SHA-256 value."});
    }

    if (!valid_manifest(request.manifest_json))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidManifest,
                                                                         "Package manifest is required."});
    }

    if (request.size_bytes <= 0)
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidSize,
                                                                         "Package archive size must be greater than zero."});
    }

    const auto key = version_key(
        request.workspace_id,
        request.package_id,
        request.version);

    if (impl_->version_id_by_package_version.find(key) !=
        impl_->version_id_by_package_version.end())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionAlreadyExists,
                                                                         "Package version already exists."});
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

  PackageVersionResult<dto::PackageVersionResponse> PackageVersionService::find_package_version(
      const dto::PackageVersionLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingWorkspace,
                                                                         "Workspace is required."});
    }

    if (request.package_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage,
                                                                         "Package is required."});
    }

    auto item = impl_->versions_by_id.find(request.version_id);

    if (item == impl_->versions_by_id.end() ||
        item->second.workspace_id != request.workspace_id ||
        item->second.package_id != request.package_id)
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound,
                                                                         "Package version not found."});
    }

    return PackageVersionResult<dto::PackageVersionResponse>::success(item->second);
  }

  PackageVersionResult<dto::PackageVersionResponse> PackageVersionService::find_package_version_by_number(
      const dto::PackageVersionByNumberRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingWorkspace,
                                                                         "Workspace is required."});
    }

    if (request.package_id.empty())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::MissingPackage,
                                                                         "Package is required."});
    }

    if (!valid_version(request.version))
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::InvalidVersion,
                                                                         "Package version is invalid."});
    }

    const auto key = version_key(
        request.workspace_id,
        request.package_id,
        request.version);

    auto version_id = impl_->version_id_by_package_version.find(key);

    if (version_id == impl_->version_id_by_package_version.end())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound,
                                                                         "Package version not found."});
    }

    auto version = impl_->versions_by_id.find(version_id->second);

    if (version == impl_->versions_by_id.end())
    {
      return PackageVersionResult<dto::PackageVersionResponse>::failure({support::PackageVersionErrorCode::PackageVersionNotFound,
                                                                         "Package version not found."});
    }

    return PackageVersionResult<dto::PackageVersionResponse>::success(version->second);
  }

  PackageVersionResult<std::vector<dto::PackageVersionResponse>> PackageVersionService::list_package_versions(
      const dto::ListPackageVersionsRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::failure({support::PackageVersionErrorCode::MissingWorkspace,
                                                                                      "Workspace is required."});
    }

    if (request.package_id.empty())
    {
      return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::failure({support::PackageVersionErrorCode::MissingPackage,
                                                                                      "Package is required."});
    }

    std::vector<dto::PackageVersionResponse> versions;

    for (const auto &entry : impl_->versions_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id &&
          entry.second.package_id == request.package_id)
      {
        versions.push_back(entry.second);
      }
    }

    std::sort(
        versions.begin(),
        versions.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return PackageVersionResult<std::vector<dto::PackageVersionResponse>>::success(versions);
  }
} // namespace cloud::package_versions::services
