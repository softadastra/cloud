/**
 *
 *  @file PackageVersionService.hpp
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
#ifndef cloud_package_versions_services_package_version_service_hpp
#define cloud_package_versions_services_package_version_service_hpp

#include <package_versions/dto/PackageVersionRequests.hpp>
#include <package_versions/dto/PackageVersionResponses.hpp>
#include <package_versions/support/PackageVersionErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::package_versions::services
{
  template <typename T>
  class PackageVersionResult
  {
  public:
    static PackageVersionResult success(T value)
    {
      PackageVersionResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static PackageVersionResult failure(
        support::PackageVersionError error)
    {
      PackageVersionResult result;
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

    const support::PackageVersionError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::PackageVersionError error_{};
  };

  class PackageVersionService
  {
  public:
    PackageVersionService();
    ~PackageVersionService();

    PackageVersionService(const PackageVersionService &) = delete;
    PackageVersionService &operator=(const PackageVersionService &) = delete;

    PackageVersionService(PackageVersionService &&) noexcept;
    PackageVersionService &operator=(PackageVersionService &&) noexcept;

    PackageVersionResult<dto::PackageVersionResponse> publish_package_version(
        const dto::PublishPackageVersionRequest &request);

    PackageVersionResult<dto::PackageVersionResponse> find_package_version(
        const dto::PackageVersionLookupRequest &request) const;

    PackageVersionResult<dto::PackageVersionResponse> find_package_version_by_number(
        const dto::PackageVersionByNumberRequest &request) const;

    PackageVersionResult<dto::PackageVersionResponse> set_status(
        const dto::PackageVersionLookupRequest &request,
        const std::string &status,
        const std::string &deprecation_message = "");

    PackageVersionResult<std::vector<dto::PackageVersionResponse>> list_package_versions(
        const dto::ListPackageVersionsRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::package_versions::services

#endif // cloud_package_versions_services_package_version_service_hpp
