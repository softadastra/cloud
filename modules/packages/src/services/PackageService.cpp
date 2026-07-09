/**
 *
 *  @file PackageService.cpp
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
#include <packages/services/PackageService.hpp>

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

namespace cloud::packages::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_package_id()
    {
      static std::atomic<std::uint64_t> counter{0};
      std::ostringstream stream;
      stream << "package_" << now_timestamp() << "_" << counter.fetch_add(1);
      return stream.str();
    }

    std::string normalize_package_name(const std::string &value)
    {
      std::string name;
      for (const char ch : value)
      {
        const auto c = static_cast<unsigned char>(ch);
        if (std::isalnum(c))
        {
          name.push_back(static_cast<char>(std::tolower(c)));
          continue;
        }
        if (ch == '/' || ch == '-' || ch == '_' || ch == '.')
        {
          name.push_back(ch);
        }
      }
      return name;
    }

    bool valid_package_name(const std::string &name)
    {
      if (name.size() < 2 || name.size() > 120 || name.front() == '/' || name.back() == '/')
      {
        return false;
      }
      bool previous_slash = false;
      for (const char ch : name)
      {
        const auto c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || ch == '-' || ch == '_' || ch == '.')
        {
          previous_slash = false;
          continue;
        }
        if (ch == '/')
        {
          if (previous_slash)
          {
            return false;
          }
          previous_slash = true;
          continue;
        }
        return false;
      }
      return true;
    }

    std::string normalize_visibility(const std::string &visibility)
    {
      return visibility.empty() ? "private" : visibility;
    }

    bool valid_visibility(const std::string &visibility)
    {
      return visibility == "private" || visibility == "public";
    }

    std::string package_key(const std::string &workspace_id, const std::string &name)
    {
      return workspace_id + "::" + name;
    }
  } // namespace

  class PackageService::Impl
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

    bool workspace_exists(const std::string &workspace_id) const
    {
      auto rows = db->query("SELECT id FROM workspaces WHERE id = ? LIMIT 1", workspace_id);
      return rows->next();
    }

    bool package_name_exists(const std::string &workspace_id, const std::string &name, const std::string &except_id = "") const
    {
      auto rows = db->query("SELECT id FROM packages WHERE workspace_id = ? AND name = ? LIMIT 1", workspace_id, name);
      if (!rows->next())
      {
        return false;
      }
      return except_id.empty() || rows->row().getString(0) != except_id;
    }

    dto::PackageResponse row_to_package(const vix::db::ResultRow &row) const
    {
      dto::PackageResponse package;
      package.id = row.getString(0);
      package.workspace_id = row.getString(1);
      package.owner_user_id = row.getString(2);
      package.name = row.getString(3);
      package.description = row.getString(4);
      package.repository_url = row.getString(5);
      package.visibility = row.getString(6);
      package.active = row.getInt64(7) != 0;
      package.created_at = row.getInt64(8);
      package.updated_at = row.getInt64(9);
      return package;
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::PackageResponse> packages_by_id;
    std::unordered_map<std::string, std::string> package_id_by_workspace_name;
  };

  PackageService::PackageService() : impl_(std::make_unique<Impl>()) {}
  PackageService::~PackageService() = default;
  PackageService::PackageService(PackageService &&) noexcept = default;
  PackageService &PackageService::operator=(PackageService &&) noexcept = default;

  PackageResult<dto::PackageResponse> PackageService::create_package(const dto::CreatePackageRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.owner_user_id.empty())
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::MissingOwner, "Package owner is required."});
    }

    const auto name = normalize_package_name(request.name);
    if (!valid_package_name(name))
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::InvalidName, "Package name must contain only letters, numbers, slash, dash, underscore or dot."});
    }

    const auto visibility = normalize_visibility(request.visibility);
    if (!valid_visibility(visibility))
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::InvalidVisibility, "Package visibility must be private or public."});
    }

    if (impl_->persistent())
    {
      try
      {
        if (!impl_->workspace_exists(request.workspace_id))
        {
          return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::MissingWorkspace, "Workspace not found."});
        }
        if (impl_->package_name_exists(request.workspace_id, name))
        {
          return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageAlreadyExists, "Package already exists in this workspace."});
        }

        const auto timestamp = now_timestamp();
        dto::PackageResponse package;
        package.id = make_package_id();
        package.workspace_id = request.workspace_id;
        package.owner_user_id = request.owner_user_id;
        package.name = name;
        package.description = request.description;
        package.repository_url = request.repository_url;
        package.visibility = visibility;
        package.active = true;
        package.created_at = timestamp;
        package.updated_at = timestamp;

        impl_->db->exec(
            "INSERT INTO packages (id, workspace_id, owner_user_id, name, description, repository_url, visibility, active, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            package.id, package.workspace_id, package.owner_user_id, package.name, package.description, package.repository_url, package.visibility,
            static_cast<std::int64_t>(1), package.created_at, package.updated_at);

        return PackageResult<dto::PackageResponse>::success(package);
      }
      catch (const std::exception &error)
      {
        (void)error;
        return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageAlreadyExists, "Package already exists in this workspace."});
      }
    }

    const auto key = package_key(request.workspace_id, name);
    if (impl_->package_id_by_workspace_name.find(key) != impl_->package_id_by_workspace_name.end())
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageAlreadyExists, "Package already exists in this workspace."});
    }

    const auto timestamp = now_timestamp();
    dto::PackageResponse package;
    package.id = make_package_id();
    package.workspace_id = request.workspace_id;
    package.owner_user_id = request.owner_user_id;
    package.name = name;
    package.description = request.description;
    package.repository_url = request.repository_url;
    package.visibility = visibility;
    package.active = true;
    package.created_at = timestamp;
    package.updated_at = timestamp;

    impl_->package_id_by_workspace_name[key] = package.id;
    impl_->packages_by_id[package.id] = package;
    return PackageResult<dto::PackageResponse>::success(package);
  }

  PackageResult<dto::PackageResponse> PackageService::update_package(const dto::UpdatePackageRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::MissingWorkspace, "Workspace is required."});
    }

    if (impl_->persistent())
    {
      auto current = find_package({request.id, request.workspace_id});
      if (current.failed())
      {
        return current;
      }

      const auto name = normalize_package_name(request.name);
      if (!valid_package_name(name))
      {
        return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::InvalidName, "Package name must contain only letters, numbers, slash, dash, underscore or dot."});
      }
      const auto visibility = normalize_visibility(request.visibility);
      if (!valid_visibility(visibility))
      {
        return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::InvalidVisibility, "Package visibility must be private or public."});
      }
      if (impl_->package_name_exists(request.workspace_id, name, request.id))
      {
        return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageAlreadyExists, "Package already exists in this workspace."});
      }

      auto package = current.value();
      package.name = name;
      package.description = request.description;
      package.repository_url = request.repository_url;
      package.visibility = visibility;
      package.updated_at = now_timestamp();

      impl_->db->exec(
          "UPDATE packages SET name = ?, description = ?, repository_url = ?, visibility = ?, updated_at = ? WHERE id = ? AND workspace_id = ?",
          package.name, package.description, package.repository_url, package.visibility, package.updated_at, package.id, package.workspace_id);

      return PackageResult<dto::PackageResponse>::success(package);
    }

    auto item = impl_->packages_by_id.find(request.id);
    if (item == impl_->packages_by_id.end() || item->second.workspace_id != request.workspace_id)
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageNotFound, "Package not found."});
    }

    const auto name = normalize_package_name(request.name);
    if (!valid_package_name(name))
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::InvalidName, "Package name must contain only letters, numbers, slash, dash, underscore or dot."});
    }
    const auto visibility = normalize_visibility(request.visibility);
    if (!valid_visibility(visibility))
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::InvalidVisibility, "Package visibility must be private or public."});
    }

    const auto new_key = package_key(request.workspace_id, name);
    auto existing = impl_->package_id_by_workspace_name.find(new_key);
    if (existing != impl_->package_id_by_workspace_name.end() && existing->second != request.id)
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageAlreadyExists, "Package already exists in this workspace."});
    }

    impl_->package_id_by_workspace_name.erase(package_key(item->second.workspace_id, item->second.name));
    item->second.name = name;
    item->second.description = request.description;
    item->second.repository_url = request.repository_url;
    item->second.visibility = visibility;
    item->second.updated_at = now_timestamp();
    impl_->package_id_by_workspace_name[new_key] = item->second.id;
    return PackageResult<dto::PackageResponse>::success(item->second);
  }

  PackageResult<dto::PackageResponse> PackageService::find_package(const dto::PackageLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::MissingWorkspace, "Workspace is required."});
    }

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, owner_user_id, name, description, repository_url, visibility, active, created_at, updated_at FROM packages WHERE id = ? AND workspace_id = ? LIMIT 1",
          request.id, request.workspace_id);
      if (!rows->next())
      {
        return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageNotFound, "Package not found."});
      }
      return PackageResult<dto::PackageResponse>::success(impl_->row_to_package(rows->row()));
    }

    auto item = impl_->packages_by_id.find(request.id);
    if (item == impl_->packages_by_id.end() || item->second.workspace_id != request.workspace_id)
    {
      return PackageResult<dto::PackageResponse>::failure({support::PackageErrorCode::PackageNotFound, "Package not found."});
    }
    return PackageResult<dto::PackageResponse>::success(item->second);
  }

  PackageResult<std::vector<dto::PackageResponse>> PackageService::list_packages(const dto::ListPackagesRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return PackageResult<std::vector<dto::PackageResponse>>::failure({support::PackageErrorCode::MissingWorkspace, "Workspace is required."});
    }

    std::vector<dto::PackageResponse> packages;
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, owner_user_id, name, description, repository_url, visibility, active, created_at, updated_at FROM packages WHERE workspace_id = ? ORDER BY created_at",
          request.workspace_id);
      while (rows->next())
      {
        packages.push_back(impl_->row_to_package(rows->row()));
      }
      return PackageResult<std::vector<dto::PackageResponse>>::success(packages);
    }

    for (const auto &entry : impl_->packages_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id)
      {
        packages.push_back(entry.second);
      }
    }
    std::sort(packages.begin(), packages.end(), [](const auto &left, const auto &right) { return left.created_at < right.created_at; });
    return PackageResult<std::vector<dto::PackageResponse>>::success(packages);
  }
} // namespace cloud::packages::services
