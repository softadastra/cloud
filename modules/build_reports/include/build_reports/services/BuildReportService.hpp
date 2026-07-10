/**
 *
 *  @file BuildReportService.hpp
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
#ifndef cloud_build_reports_services_build_report_service_hpp
#define cloud_build_reports_services_build_report_service_hpp

#include <build_reports/dto/BuildReportRequests.hpp>
#include <build_reports/dto/BuildReportResponses.hpp>
#include <build_reports/support/BuildReportErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::build_reports::services
{
  template <typename T>
  class BuildReportResult
  {
  public:
    static BuildReportResult success(T value)
    {
      BuildReportResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static BuildReportResult failure(
        support::BuildReportError error)
    {
      BuildReportResult result;
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

    const support::BuildReportError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::BuildReportError error_{};
  };

  class BuildReportService
  {
  public:
    BuildReportService();
    ~BuildReportService();

    BuildReportService(const BuildReportService &) = delete;
    BuildReportService &operator=(const BuildReportService &) = delete;

    BuildReportService(BuildReportService &&) noexcept;
    BuildReportService &operator=(BuildReportService &&) noexcept;

    BuildReportResult<dto::BuildReportResponse> submit_build_report(
        const dto::SubmitBuildReportRequest &request);

    BuildReportResult<dto::BuildReportResponse> find_build_report(
        const dto::BuildReportLookupRequest &request) const;

    BuildReportResult<dto::BuildReportResponse> set_record_status(
        const dto::BuildReportLookupRequest &request,
        const std::string &record_status);

    BuildReportResult<std::vector<dto::BuildReportResponse>> list_build_reports(
        const dto::ListBuildReportsRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::build_reports::services

#endif // cloud_build_reports_services_build_report_service_hpp
