/**
 *
 *  @file BuildReportResponses.hpp
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
#ifndef cloud_build_reports_dto_build_report_responses_hpp
#define cloud_build_reports_dto_build_report_responses_hpp

#include <cstdint>
#include <string>

#include <vix/json.hpp>

namespace cloud::build_reports::dto
{
  struct BuildReportResponse
  {
    std::string id;
    std::string workspace_id;
    std::string project_id;
    std::string submitted_by_user_id;
    std::string status;
    std::string target;
    std::string profile;
    std::string branch;
    std::string commit_sha;
    std::string toolchain;
    std::string summary_json;
    std::string diagnostics_json;
    std::int64_t duration_ms{0};
    std::int64_t warnings_count{0};
    std::int64_t errors_count{0};
    std::int64_t created_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "project_id", project_id,
          "submitted_by_user_id", submitted_by_user_id,
          "status", status,
          "target", target,
          "profile", profile,
          "branch", branch,
          "commit_sha", commit_sha,
          "toolchain", toolchain,
          "summary_json", summary_json,
          "diagnostics_json", diagnostics_json,
          "duration_ms", duration_ms,
          "warnings_count", warnings_count,
          "errors_count", errors_count,
          "created_at", created_at);
    }
  };
} // namespace cloud::build_reports::dto

#endif // cloud_build_reports_dto_build_report_responses_hpp
