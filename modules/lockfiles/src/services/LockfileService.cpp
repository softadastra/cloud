/**
 *
 *  @file LockfileService.cpp
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
#include <lockfiles/services/LockfileService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::lockfiles::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_lockfile_id()
    {
      static std::atomic<std::uint64_t> counter{0};
      std::ostringstream stream;
      stream << "lockfile_" << now_timestamp() << "_" << counter.fetch_add(1);
      return stream.str();
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

    std::string normalize_source(const std::string &source)
    {
      return source.empty() ? "vix" : source;
    }
  } // namespace

  class LockfileService::Impl
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

    bool project_exists(const std::string &workspace_id, const std::string &project_id) const
    {
      auto rows = db->query("SELECT id FROM projects WHERE id = ? AND workspace_id = ? LIMIT 1", project_id, workspace_id);
      return rows->next();
    }

    dto::LockfileResponse row_to_lockfile(const vix::db::ResultRow &row) const
    {
      dto::LockfileResponse lockfile;
      lockfile.id = row.getString(0);
      lockfile.workspace_id = row.getString(1);
      lockfile.project_id = row.getString(2);
      lockfile.uploaded_by_user_id = row.getString(3);
      lockfile.checksum_sha256 = row.getString(4);
      lockfile.lockfile_json = row.getString(5);
      lockfile.source = row.getString(6);
      lockfile.created_at = row.getInt64(7);
      lockfile.status = row.getString(8);
      lockfile.deleted_at = row.getInt64(9);
      return lockfile;
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::LockfileResponse> lockfiles_by_id;
  };

  LockfileService::LockfileService() : impl_(std::make_unique<Impl>()) {}
  LockfileService::~LockfileService() = default;
  LockfileService::LockfileService(LockfileService &&) noexcept = default;
  LockfileService &LockfileService::operator=(LockfileService &&) noexcept = default;

  LockfileResult<dto::LockfileResponse> LockfileService::upload_lockfile(const dto::UploadLockfileRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.project_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingProject, "Project is required."});
    }
    if (request.uploaded_by_user_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingUploader, "Uploader is required."});
    }
    if (request.lockfile_json.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingLockfile, "Lockfile content is required."});
    }
    if (!valid_checksum_sha256(request.checksum_sha256))
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::InvalidChecksum, "Lockfile checksum must be a valid SHA-256 value."});
    }

    if (impl_->persistent())
    {
      if (!impl_->project_exists(request.workspace_id, request.project_id))
      {
        return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingProject, "Project not found."});
      }

      const auto timestamp = now_timestamp();
      dto::LockfileResponse lockfile;
      lockfile.id = make_lockfile_id();
      lockfile.workspace_id = request.workspace_id;
      lockfile.project_id = request.project_id;
      lockfile.uploaded_by_user_id = request.uploaded_by_user_id;
      lockfile.lockfile_json = request.lockfile_json;
      lockfile.checksum_sha256 = request.checksum_sha256;
      lockfile.source = normalize_source(request.source);
      lockfile.created_at = timestamp;

      impl_->db->exec(
          "INSERT INTO lockfiles (id, workspace_id, project_id, uploaded_by_user_id, checksum_sha256, lockfile_json, source, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
          lockfile.id, lockfile.workspace_id, lockfile.project_id, lockfile.uploaded_by_user_id, lockfile.checksum_sha256,
          lockfile.lockfile_json, lockfile.source, lockfile.created_at, lockfile.created_at);

      return LockfileResult<dto::LockfileResponse>::success(lockfile);
    }

    dto::LockfileResponse lockfile;
    lockfile.id = make_lockfile_id();
    lockfile.workspace_id = request.workspace_id;
    lockfile.project_id = request.project_id;
    lockfile.uploaded_by_user_id = request.uploaded_by_user_id;
    lockfile.lockfile_json = request.lockfile_json;
    lockfile.checksum_sha256 = request.checksum_sha256;
    lockfile.source = normalize_source(request.source);
    lockfile.created_at = now_timestamp();
    impl_->lockfiles_by_id[lockfile.id] = lockfile;
    return LockfileResult<dto::LockfileResponse>::success(lockfile);
  }

  LockfileResult<dto::LockfileResponse> LockfileService::find_lockfile(const dto::LockfileLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.project_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingProject, "Project is required."});
    }

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, project_id, uploaded_by_user_id, checksum_sha256, lockfile_json, source, created_at, COALESCE(status, 'active'), COALESCE(deleted_at, 0) FROM lockfiles WHERE id = ? AND workspace_id = ? AND project_id = ? AND COALESCE(status, 'active') != 'deleted' LIMIT 1",
          request.lockfile_id, request.workspace_id, request.project_id);
      if (!rows->next())
      {
        return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::LockfileNotFound, "Lockfile not found."});
      }
      return LockfileResult<dto::LockfileResponse>::success(impl_->row_to_lockfile(rows->row()));
    }

    auto item = impl_->lockfiles_by_id.find(request.lockfile_id);
    if (item == impl_->lockfiles_by_id.end() || item->second.workspace_id != request.workspace_id || item->second.project_id != request.project_id)
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::LockfileNotFound, "Lockfile not found."});
    }
    return LockfileResult<dto::LockfileResponse>::success(item->second);
  }



  LockfileResult<dto::LockfileResponse> LockfileService::set_status(
      const dto::LockfileLookupRequest &request,
      const std::string &status)
  {
    if (status != "active" && status != "deleted")
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::InvalidChecksum, "Lockfile status is invalid."});
    }

    auto current = find_lockfile(request);
    if (current.failed())
    {
      return current;
    }

    auto lockfile = current.value();
    lockfile.status = status;
    lockfile.deleted_at = status == "deleted" ? now_timestamp() : 0;

    if (impl_->persistent())
    {
      impl_->db->exec(
          "UPDATE lockfiles SET status = ?, deleted_at = ?, updated_at = ? WHERE id = ? AND workspace_id = ? AND project_id = ?",
          lockfile.status,
          lockfile.deleted_at,
          now_timestamp(),
          lockfile.id,
          lockfile.workspace_id,
          lockfile.project_id);
    }
    else
    {
      impl_->lockfiles_by_id[lockfile.id] = lockfile;
    }

    return LockfileResult<dto::LockfileResponse>::success(lockfile);
  }

  LockfileResult<std::vector<dto::LockfileResponse>> LockfileService::list_lockfiles(const dto::ListLockfilesRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return LockfileResult<std::vector<dto::LockfileResponse>>::failure({support::LockfileErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.project_id.empty())
    {
      return LockfileResult<std::vector<dto::LockfileResponse>>::failure({support::LockfileErrorCode::MissingProject, "Project is required."});
    }

    std::vector<dto::LockfileResponse> lockfiles;
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, project_id, uploaded_by_user_id, checksum_sha256, lockfile_json, source, created_at, COALESCE(status, 'active'), COALESCE(deleted_at, 0) FROM lockfiles WHERE workspace_id = ? AND project_id = ? AND COALESCE(status, 'active') != 'deleted' ORDER BY created_at",
          request.workspace_id, request.project_id);
      while (rows->next())
      {
        lockfiles.push_back(impl_->row_to_lockfile(rows->row()));
      }
      return LockfileResult<std::vector<dto::LockfileResponse>>::success(lockfiles);
    }

    for (const auto &entry : impl_->lockfiles_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id && entry.second.project_id == request.project_id)
      {
        lockfiles.push_back(entry.second);
      }
    }
    std::sort(lockfiles.begin(), lockfiles.end(), [](const auto &left, const auto &right) { return left.created_at < right.created_at; });
    return LockfileResult<std::vector<dto::LockfileResponse>>::success(lockfiles);
  }
} // namespace cloud::lockfiles::services
