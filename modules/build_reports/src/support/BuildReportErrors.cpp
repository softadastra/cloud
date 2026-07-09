/**
 *
 *  @file BuildReportErrors.cpp
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
#include <build_reports/support/BuildReportErrors.hpp>

#include <vix/json.hpp>

namespace cloud::build_reports::support
{
  int http_status_for_build_report_error(
      const BuildReportError &error)
  {
    switch (error.code)
    {
    case BuildReportErrorCode::InvalidRequest:
    case BuildReportErrorCode::MissingWorkspace:
    case BuildReportErrorCode::MissingProject:
    case BuildReportErrorCode::MissingSubmitter:
    case BuildReportErrorCode::InvalidStatus:
    case BuildReportErrorCode::InvalidTarget:
    case BuildReportErrorCode::InvalidSummary:
    case BuildReportErrorCode::InvalidDiagnostics:
    case BuildReportErrorCode::InvalidDuration:
      return 400;

    case BuildReportErrorCode::Unauthorized:
      return 401;

    case BuildReportErrorCode::BuildReportNotFound:
      return 404;

    case BuildReportErrorCode::InternalError:
    default:
      return 500;
    }
  }

  std::string public_code_for_build_report_error(
      const BuildReportError &error)
  {
    switch (error.code)
    {
    case BuildReportErrorCode::InvalidRequest:
      return "invalid_request";

    case BuildReportErrorCode::MissingWorkspace:
      return "missing_workspace";

    case BuildReportErrorCode::MissingProject:
      return "missing_project";

    case BuildReportErrorCode::MissingSubmitter:
      return "missing_submitter";

    case BuildReportErrorCode::InvalidStatus:
      return "invalid_build_status";

    case BuildReportErrorCode::InvalidTarget:
      return "invalid_build_target";

    case BuildReportErrorCode::InvalidSummary:
      return "invalid_build_summary";

    case BuildReportErrorCode::InvalidDiagnostics:
      return "invalid_build_diagnostics";

    case BuildReportErrorCode::InvalidDuration:
      return "invalid_build_duration";

    case BuildReportErrorCode::Unauthorized:
      return "unauthorized";

    case BuildReportErrorCode::BuildReportNotFound:
      return "build_report_not_found";

    case BuildReportErrorCode::InternalError:
    default:
      return "build_report_error";
    }
  }

  std::string public_message_for_build_report_error(
      const BuildReportError &error)
  {
    if (!error.message.empty())
    {
      return error.message;
    }

    switch (error.code)
    {
    case BuildReportErrorCode::InvalidRequest:
      return "Invalid build report request.";

    case BuildReportErrorCode::MissingWorkspace:
      return "Workspace is required.";

    case BuildReportErrorCode::MissingProject:
      return "Project is required.";

    case BuildReportErrorCode::MissingSubmitter:
      return "Submitter is required.";

    case BuildReportErrorCode::InvalidStatus:
      return "Invalid build status.";

    case BuildReportErrorCode::InvalidTarget:
      return "Build target is required.";

    case BuildReportErrorCode::InvalidSummary:
      return "Build summary is invalid.";

    case BuildReportErrorCode::InvalidDiagnostics:
      return "Build diagnostics are invalid.";

    case BuildReportErrorCode::InvalidDuration:
      return "Build duration is invalid.";

    case BuildReportErrorCode::Unauthorized:
      return "Authentication is required.";

    case BuildReportErrorCode::BuildReportNotFound:
      return "Build report not found.";

    case BuildReportErrorCode::InternalError:
    default:
      return "Build report operation failed.";
    }
  }

  void write_build_report_error(
      vix::Response &res,
      const BuildReportError &error)
  {
    res.status(http_status_for_build_report_error(error)).json(vix::json::o("ok", false, "error", public_code_for_build_report_error(error), "message", public_message_for_build_report_error(error)));
  }
} // namespace cloud::build_reports::support
