/**
 *
 *  @file LockfileService.hpp
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
#ifndef cloud_lockfiles_services_lockfile_service_hpp
#define cloud_lockfiles_services_lockfile_service_hpp

#include <lockfiles/dto/LockfileRequests.hpp>
#include <lockfiles/dto/LockfileResponses.hpp>
#include <lockfiles/support/LockfileErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::lockfiles::services
{
  template <typename T>
  class LockfileResult
  {
  public:
    static LockfileResult success(T value)
    {
      LockfileResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static LockfileResult failure(
        support::LockfileError error)
    {
      LockfileResult result;
      result.ok_ = false;
      result.error_ = std::move(error);
      return result;
    }

    bool ok() const
    {
      return ok_;
    }

    bool failed() const
    {
      return !ok_;
    }

    const T &value() const
    {
      return value_;
    }

    const support::LockfileError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::LockfileError error_{};
  };

  class LockfileService
  {
  public:
    LockfileService();
    ~LockfileService();

    LockfileService(const LockfileService &) = delete;
    LockfileService &operator=(const LockfileService &) = delete;

    LockfileService(LockfileService &&) noexcept;
    LockfileService &operator=(LockfileService &&) noexcept;

    LockfileResult<dto::LockfileResponse> upload_lockfile(
        const dto::UploadLockfileRequest &request);

    LockfileResult<dto::LockfileResponse> find_lockfile(
        const dto::LockfileLookupRequest &request) const;

    LockfileResult<dto::LockfileResponse> set_status(
        const dto::LockfileLookupRequest &request,
        const std::string &status);

    LockfileResult<std::vector<dto::LockfileResponse>> list_lockfiles(
        const dto::ListLockfilesRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::lockfiles::services

#endif // cloud_lockfiles_services_lockfile_service_hpp
