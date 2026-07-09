/**
 *
 *  @file BuildReportService.cpp
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
#include <build_reports/services/BuildReportService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace cloud::build_reports::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_build_report_id()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "build_report_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    bool valid_status(
        const std::string &status)
    {
      return status == "success" ||
             status == "failed" ||
             status == "warning" ||
             status == "cancelled";
    }

    bool valid_target(
        const std::string &target)
    {
      return !target.empty() && target.size() <= 120;
    }

    bool valid_json_text(
        const std::string &value)
    {
      return !value.empty();
    }

    std::string normalize_status(
        const std::string &status)
    {
      if (status.empty())
      {
        return "success";
      }

      return status;
    }

    std::string normalize_profile(
        const std::string &profile)
    {
      if (profile.empty())
      {
        return "dev";
      }

      return profile;
    }

    std::string normalize_summary_json(
        const std::string &summary_json)
    {
      if (summary_json.empty())
      {
        return "{}";
      }

      return summary_json;
    }

    std::string normalize_diagnostics_json(
        const std::string &diagnostics_json)
    {
      if (diagnostics_json.empty())
      {
        return "[]";
      }

      return diagnostics_json;
    }
  } // namespace

  class BuildReportService::Impl
  {
  public:
    std::unordered_map<std::string, dto::BuildReportResponse> reports_by_id;
  };

  BuildReportService::BuildReportService()
      : impl_(std::make_unique<Impl>())
  {
  }

  BuildReportService::~BuildReportService() = default;

  BuildReportService::BuildReportService(BuildReportService &&) noexcept = default;

  BuildReportService &BuildReportService::operator=(BuildReportService &&) noexcept = default;

  BuildReportResult<dto::BuildReportResponse> BuildReportService::submit_build_report(
      const dto::SubmitBuildReportRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingWorkspace,
                                                                   "Workspace is required."});
    }

    if (request.project_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingProject,
                                                                   "Project is required."});
    }

    if (request.submitted_by_user_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingSubmitter,
                                                                   "Submitter is required."});
    }

    const auto status = normalize_status(request.status);

    if (!valid_status(status))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidStatus,
                                                                   "Build status must be success, failed, warning or cancelled."});
    }

    if (!valid_target(request.target))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidTarget,
                                                                   "Build target is required."});
    }

    const auto summary_json = normalize_summary_json(request.summary_json);

    if (!valid_json_text(summary_json))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidSummary,
                                                                   "Build summary is invalid."});
    }

    const auto diagnostics_json = normalize_diagnostics_json(request.diagnostics_json);

    if (!valid_json_text(diagnostics_json))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidDiagnostics,
                                                                   "Build diagnostics are invalid."});
    }

    if (request.duration_ms < 0 ||
        request.warnings_count < 0 ||
        request.errors_count < 0)
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidDuration,
                                                                   "Build report counters must not be negative."});
    }

    dto::BuildReportResponse report;
    report.id = make_build_report_id();
    report.workspace_id = request.workspace_id;
    report.project_id = request.project_id;
    report.submitted_by_user_id = request.submitted_by_user_id;
    report.status = status;
    report.target = request.target;
    report.profile = normalize_profile(request.profile);
    report.branch = request.branch;
    report.commit_sha = request.commit_sha;
    report.toolchain = request.toolchain;
    report.summary_json = summary_json;
    report.diagnostics_json = diagnostics_json;
    report.duration_ms = request.duration_ms;
    report.warnings_count = request.warnings_count;
    report.errors_count = request.errors_count;
    report.created_at = now_timestamp();

    impl_->reports_by_id[report.id] = report;

    return BuildReportResult<dto::BuildReportResponse>::success(report);
  }

  BuildReportResult<dto::BuildReportResponse> BuildReportService::find_build_report(
      const dto::BuildReportLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingWorkspace,
                                                                   "Workspace is required."});
    }

    if (request.project_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingProject,
                                                                   "Project is required."});
    }

    auto item = impl_->reports_by_id.find(request.build_report_id);

    if (item == impl_->reports_by_id.end() ||
        item->second.workspace_id != request.workspace_id ||
        item->second.project_id != request.project_id)
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::BuildReportNotFound,
                                                                   "Build report not found."});
    }

    return BuildReportResult<dto::BuildReportResponse>::success(item->second);
  }

  BuildReportResult<std::vector<dto::BuildReportResponse>> BuildReportService::list_build_reports(
      const dto::ListBuildReportsRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return BuildReportResult<std::vector<dto::BuildReportResponse>>::failure({support::BuildReportErrorCode::MissingWorkspace,
                                                                                "Workspace is required."});
    }

    if (request.project_id.empty())
    {
      return BuildReportResult<std::vector<dto::BuildReportResponse>>::failure({support::BuildReportErrorCode::MissingProject,
                                                                                "Project is required."});
    }

    std::vector<dto::BuildReportResponse> reports;

    for (const auto &entry : impl_->reports_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id &&
          entry.second.project_id == request.project_id)
      {
        reports.push_back(entry.second);
      }
    }

    std::sort(
        reports.begin(),
        reports.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return BuildReportResult<std::vector<dto::BuildReportResponse>>::success(reports);
  }
} // namespace cloud::build_reports::services
