/**
 *
 *  @file BuildReportErrors.hpp
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
#ifndef cloud_build_reports_support_build_report_errors_hpp
#define cloud_build_reports_support_build_report_errors_hpp

#include <string>

#include <vix.hpp>

namespace cloud::build_reports::support
{
  enum class BuildReportErrorCode
  {
    InvalidRequest,
    MissingWorkspace,
    MissingProject,
    MissingSubmitter,
    InvalidStatus,
    InvalidTarget,
    InvalidSummary,
    InvalidDiagnostics,
    InvalidDuration,
    BuildReportNotFound,
    Unauthorized,
    InternalError
  };

  struct BuildReportError
  {
    BuildReportErrorCode code{BuildReportErrorCode::InternalError};
    std::string message;
  };

  int http_status_for_build_report_error(
      const BuildReportError &error);

  std::string public_code_for_build_report_error(
      const BuildReportError &error);

  std::string public_message_for_build_report_error(
      const BuildReportError &error);

  void write_build_report_error(
      vix::Response &res,
      const BuildReportError &error);
} // namespace cloud::build_reports::support

#endif // cloud_build_reports_support_build_report_errors_hpp
