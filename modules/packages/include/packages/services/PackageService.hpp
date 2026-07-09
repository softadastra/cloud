/**
 *
 *  @file PackageService.hpp
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
#ifndef cloud_packages_services_package_service_hpp
#define cloud_packages_services_package_service_hpp

#include <packages/dto/PackageRequests.hpp>
#include <packages/dto/PackageResponses.hpp>
#include <packages/support/PackageErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::packages::services
{
  template <typename T>
  class PackageResult
  {
  public:
    static PackageResult success(T value)
    {
      PackageResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static PackageResult failure(
        support::PackageError error)
    {
      PackageResult result;
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

    const support::PackageError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::PackageError error_{};
  };

  class PackageService
  {
  public:
    PackageService();
    ~PackageService();

    PackageService(const PackageService &) = delete;
    PackageService &operator=(const PackageService &) = delete;

    PackageService(PackageService &&) noexcept;
    PackageService &operator=(PackageService &&) noexcept;

    PackageResult<dto::PackageResponse> create_package(
        const dto::CreatePackageRequest &request);

    PackageResult<dto::PackageResponse> update_package(
        const dto::UpdatePackageRequest &request);

    PackageResult<dto::PackageResponse> find_package(
        const dto::PackageLookupRequest &request) const;

    PackageResult<std::vector<dto::PackageResponse>> list_packages(
        const dto::ListPackagesRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::packages::services

#endif // cloud_packages_services_package_service_hpp
