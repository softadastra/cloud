/**
 *
 *  @file BuildReportRequests.hpp
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
#ifndef cloud_build_reports_dto_build_report_requests_hpp
#define cloud_build_reports_dto_build_report_requests_hpp

#include <cstdint>
#include <string>

namespace cloud::build_reports::dto
{
  struct SubmitBuildReportRequest
  {
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
  };

  struct BuildReportLookupRequest
  {
    std::string workspace_id;
    std::string project_id;
    std::string build_report_id;
  };

  struct ListBuildReportsRequest
  {
    std::string workspace_id;
    std::string project_id;
  };
} // namespace cloud::build_reports::dto

#endif // cloud_build_reports_dto_build_report_requests_hpp
