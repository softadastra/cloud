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
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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
      stream << "lockfile_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
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

    std::string normalize_source(
        const std::string &source)
    {
      if (source.empty())
      {
        return "vix";
      }

      return source;
    }
  } // namespace

  class LockfileService::Impl
  {
  public:
    std::unordered_map<std::string, dto::LockfileResponse> lockfiles_by_id;
  };

  LockfileService::LockfileService()
      : impl_(std::make_unique<Impl>())
  {
  }

  LockfileService::~LockfileService() = default;

  LockfileService::LockfileService(LockfileService &&) noexcept = default;

  LockfileService &LockfileService::operator=(LockfileService &&) noexcept = default;

  LockfileResult<dto::LockfileResponse> LockfileService::upload_lockfile(
      const dto::UploadLockfileRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingWorkspace,
                                                             "Workspace is required."});
    }

    if (request.project_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingProject,
                                                             "Project is required."});
    }

    if (request.uploaded_by_user_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingUploader,
                                                             "Uploader is required."});
    }

    if (request.lockfile_json.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingLockfile,
                                                             "Lockfile content is required."});
    }

    if (!valid_checksum_sha256(request.checksum_sha256))
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::InvalidChecksum,
                                                             "Lockfile checksum must be a valid SHA-256 value."});
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

  LockfileResult<dto::LockfileResponse> LockfileService::find_lockfile(
      const dto::LockfileLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingWorkspace,
                                                             "Workspace is required."});
    }

    if (request.project_id.empty())
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::MissingProject,
                                                             "Project is required."});
    }

    auto item = impl_->lockfiles_by_id.find(request.lockfile_id);

    if (item == impl_->lockfiles_by_id.end() ||
        item->second.workspace_id != request.workspace_id ||
        item->second.project_id != request.project_id)
    {
      return LockfileResult<dto::LockfileResponse>::failure({support::LockfileErrorCode::LockfileNotFound,
                                                             "Lockfile not found."});
    }

    return LockfileResult<dto::LockfileResponse>::success(item->second);
  }

  LockfileResult<std::vector<dto::LockfileResponse>> LockfileService::list_lockfiles(
      const dto::ListLockfilesRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return LockfileResult<std::vector<dto::LockfileResponse>>::failure({support::LockfileErrorCode::MissingWorkspace,
                                                                          "Workspace is required."});
    }

    if (request.project_id.empty())
    {
      return LockfileResult<std::vector<dto::LockfileResponse>>::failure({support::LockfileErrorCode::MissingProject,
                                                                          "Project is required."});
    }

    std::vector<dto::LockfileResponse> lockfiles;

    for (const auto &entry : impl_->lockfiles_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id &&
          entry.second.project_id == request.project_id)
      {
        lockfiles.push_back(entry.second);
      }
    }

    std::sort(
        lockfiles.begin(),
        lockfiles.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return LockfileResult<std::vector<dto::LockfileResponse>>::success(lockfiles);
  }
} // namespace cloud::lockfiles::services
