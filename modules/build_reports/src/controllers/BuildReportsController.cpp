/**
 *
 *  @file BuildReportsController.cpp
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
#include <build_reports/controllers/BuildReportsController.hpp>

#include <build_reports/dto/BuildReportRequests.hpp>
#include <build_reports/services/BuildReportService.hpp>
#include <build_reports/support/BuildReportErrors.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::build_reports::controllers
{
  namespace
  {
    services::BuildReportService &build_report_service()
    {
      static services::BuildReportService service;
      return service;
    }

    void json_error(
        vix::Response &res,
        int status,
        const std::string &code,
        const std::string &message)
    {
      res.status(status).json(
          vix::json::o(
              "ok", false,
              "error", code,
              "message", message));
    }

    void json_ok(
        vix::Response &res,
        const vix::json::Json &data)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", data));
    }

    void json_message(
        vix::Response &res,
        const std::string &message)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "message", message)));
    }

    bool require_json_object(
        const vix::json::Json &body,
        vix::Response &res)
    {
      if (body.is_object())
      {
        return true;
      }

      json_error(
          res,
          400,
          "invalid_request",
          "Expected JSON object body.");

      return false;
    }

    dto::SubmitBuildReportRequest read_submit_build_report_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("project_id", ""),
          body.value("submitted_by_user_id", ""),
          body.value("status", ""),
          body.value("target", ""),
          body.value("profile", ""),
          body.value("branch", ""),
          body.value("commit_sha", ""),
          body.value("toolchain", ""),
          body.value("summary_json", ""),
          body.value("diagnostics_json", ""),
          body.value("duration_ms", static_cast<std::int64_t>(0)),
          body.value("warnings_count", static_cast<std::int64_t>(0)),
          body.value("errors_count", static_cast<std::int64_t>(0)),
      };
    }

    dto::BuildReportLookupRequest read_build_report_lookup_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("project_id", ""),
          body.value("build_report_id", ""),
      };
    }

    dto::ListBuildReportsRequest read_list_build_reports_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("project_id", ""),
      };
    }

    vix::json::Json build_report_list_to_json(
        const std::vector<dto::BuildReportResponse> &reports)
    {
      auto items = vix::json::a();

      for (const auto &report : reports)
      {
        items.push_back(report.to_json());
      }

      return items;
    }
  } // namespace

  void BuildReportsController::register_routes(vix::App &app)
  {
    app.get("/api/build_reports", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Build reports module is available"); });

    app.post("/api/build_reports/submit", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_submit_build_report_request(body);
      auto submitted = build_report_service().submit_build_report(request);

      if (submitted.failed())
      {
        support::write_build_report_error(res, submitted.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "build_report", submitted.value().to_json()))); });

    app.post("/api/build_reports/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_list_build_reports_request(body);
      auto reports = build_report_service().list_build_reports(request);

      if (reports.failed())
      {
        support::write_build_report_error(res, reports.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "build_reports", build_report_list_to_json(reports.value()))); });

    app.post("/api/build_reports/show", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_build_report_lookup_request(body);
      auto report = build_report_service().find_build_report(request);

      if (report.failed())
      {
        support::write_build_report_error(res, report.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "build_report", report.value().to_json())); });
  }
} // namespace cloud::build_reports::controllers
