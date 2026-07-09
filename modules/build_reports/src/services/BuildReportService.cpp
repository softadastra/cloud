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
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

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
      stream << "build_report_" << now_timestamp() << "_" << counter.fetch_add(1);
      return stream.str();
    }

    bool valid_status(const std::string &status)
    {
      return status == "success" || status == "failed" || status == "warning" || status == "cancelled";
    }

    bool valid_target(const std::string &target)
    {
      return !target.empty() && target.size() <= 120;
    }

    bool valid_json_text(const std::string &value)
    {
      return !value.empty();
    }

    std::string normalize_status(const std::string &status)
    {
      return status.empty() ? "success" : status;
    }

    std::string normalize_profile(const std::string &profile)
    {
      return profile.empty() ? "dev" : profile;
    }

    std::string normalize_summary_json(const std::string &summary_json)
    {
      return summary_json.empty() ? "{}" : summary_json;
    }

    std::string normalize_diagnostics_json(const std::string &diagnostics_json)
    {
      return diagnostics_json.empty() ? "[]" : diagnostics_json;
    }
  } // namespace

  class BuildReportService::Impl
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

    dto::BuildReportResponse row_to_report(const vix::db::ResultRow &row) const
    {
      dto::BuildReportResponse report;
      report.id = row.getString(0);
      report.workspace_id = row.getString(1);
      report.project_id = row.getString(2);
      report.submitted_by_user_id = row.getString(3);
      report.status = row.getString(4);
      report.target = row.getString(5);
      report.profile = row.getString(6);
      report.branch = row.getString(7);
      report.commit_sha = row.getString(8);
      report.toolchain = row.getString(9);
      report.summary_json = row.getString(10);
      report.diagnostics_json = row.getString(11);
      report.duration_ms = row.getInt64(12);
      report.warnings_count = row.getInt64(13);
      report.errors_count = row.getInt64(14);
      report.created_at = row.getInt64(15);
      return report;
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::BuildReportResponse> reports_by_id;
  };

  BuildReportService::BuildReportService() : impl_(std::make_unique<Impl>()) {}
  BuildReportService::~BuildReportService() = default;
  BuildReportService::BuildReportService(BuildReportService &&) noexcept = default;
  BuildReportService &BuildReportService::operator=(BuildReportService &&) noexcept = default;

  BuildReportResult<dto::BuildReportResponse> BuildReportService::submit_build_report(const dto::SubmitBuildReportRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.project_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingProject, "Project is required."});
    }
    if (request.submitted_by_user_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingSubmitter, "Submitter is required."});
    }

    const auto status = normalize_status(request.status);
    if (!valid_status(status))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidStatus, "Build status must be success, failed, warning or cancelled."});
    }
    if (!valid_target(request.target))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidTarget, "Build target is required."});
    }
    const auto summary_json = normalize_summary_json(request.summary_json);
    if (!valid_json_text(summary_json))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidSummary, "Build summary is invalid."});
    }
    const auto diagnostics_json = normalize_diagnostics_json(request.diagnostics_json);
    if (!valid_json_text(diagnostics_json))
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidDiagnostics, "Build diagnostics are invalid."});
    }
    if (request.duration_ms < 0 || request.warnings_count < 0 || request.errors_count < 0)
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::InvalidDuration, "Build report counters must not be negative."});
    }

    if (impl_->persistent())
    {
      if (!impl_->project_exists(request.workspace_id, request.project_id))
      {
        return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingProject, "Project not found."});
      }
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

    if (impl_->persistent())
    {
      impl_->db->exec(
          "INSERT INTO build_reports (id, workspace_id, project_id, submitted_by_user_id, status, target, profile, branch, commit_sha, toolchain, summary_json, diagnostics_json, duration_ms, warnings_count, errors_count, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          report.id, report.workspace_id, report.project_id, report.submitted_by_user_id, report.status, report.target, report.profile,
          report.branch, report.commit_sha, report.toolchain, report.summary_json, report.diagnostics_json, report.duration_ms,
          report.warnings_count, report.errors_count, report.created_at, report.created_at);
      return BuildReportResult<dto::BuildReportResponse>::success(report);
    }

    impl_->reports_by_id[report.id] = report;
    return BuildReportResult<dto::BuildReportResponse>::success(report);
  }

  BuildReportResult<dto::BuildReportResponse> BuildReportService::find_build_report(const dto::BuildReportLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.project_id.empty())
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::MissingProject, "Project is required."});
    }

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, project_id, submitted_by_user_id, status, target, profile, branch, commit_sha, toolchain, summary_json, diagnostics_json, duration_ms, warnings_count, errors_count, created_at FROM build_reports WHERE id = ? AND workspace_id = ? AND project_id = ? LIMIT 1",
          request.build_report_id, request.workspace_id, request.project_id);
      if (!rows->next())
      {
        return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::BuildReportNotFound, "Build report not found."});
      }
      return BuildReportResult<dto::BuildReportResponse>::success(impl_->row_to_report(rows->row()));
    }

    auto item = impl_->reports_by_id.find(request.build_report_id);
    if (item == impl_->reports_by_id.end() || item->second.workspace_id != request.workspace_id || item->second.project_id != request.project_id)
    {
      return BuildReportResult<dto::BuildReportResponse>::failure({support::BuildReportErrorCode::BuildReportNotFound, "Build report not found."});
    }
    return BuildReportResult<dto::BuildReportResponse>::success(item->second);
  }

  BuildReportResult<std::vector<dto::BuildReportResponse>> BuildReportService::list_build_reports(const dto::ListBuildReportsRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return BuildReportResult<std::vector<dto::BuildReportResponse>>::failure({support::BuildReportErrorCode::MissingWorkspace, "Workspace is required."});
    }
    if (request.project_id.empty())
    {
      return BuildReportResult<std::vector<dto::BuildReportResponse>>::failure({support::BuildReportErrorCode::MissingProject, "Project is required."});
    }

    std::vector<dto::BuildReportResponse> reports;
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, project_id, submitted_by_user_id, status, target, profile, branch, commit_sha, toolchain, summary_json, diagnostics_json, duration_ms, warnings_count, errors_count, created_at FROM build_reports WHERE workspace_id = ? AND project_id = ? ORDER BY created_at",
          request.workspace_id, request.project_id);
      while (rows->next())
      {
        reports.push_back(impl_->row_to_report(rows->row()));
      }
      return BuildReportResult<std::vector<dto::BuildReportResponse>>::success(reports);
    }

    for (const auto &entry : impl_->reports_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id && entry.second.project_id == request.project_id)
      {
        reports.push_back(entry.second);
      }
    }
    std::sort(reports.begin(), reports.end(), [](const auto &left, const auto &right) { return left.created_at < right.created_at; });
    return BuildReportResult<std::vector<dto::BuildReportResponse>>::success(reports);
  }
} // namespace cloud::build_reports::services
