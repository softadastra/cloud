/*
 * ReportController.cpp
 */

#include "modules/reports/ReportController.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include <vix/json/json.hpp>
#include <vix/log/Log.hpp>

#include "http/JsonResponse.hpp"
#include "modules/reports/ReportService.hpp"

namespace softadastra::cloud::modules::reports
{
  namespace J = vix::json;

  namespace
  {
    [[nodiscard]] bool has_string_field(
        const J::token &body,
        const std::string &field)
    {
      return body.is_object() &&
             body.contains(field) &&
             body[field].is_string();
    }

    [[nodiscard]] std::string string_field_or_empty(
        const J::token &body,
        const std::string &field)
    {
      if (!has_string_field(body, field))
      {
        return {};
      }

      return body[field].as_string_or("");
    }

    [[nodiscard]] int int_query_or(
        const vix::Request &req,
        const std::string &key,
        int fallback)
    {
      const std::string value =
          req.query_value(key, "");

      if (value.empty())
      {
        return fallback;
      }

      try
      {
        return std::stoi(value);
      }
      catch (...)
      {
        return fallback;
      }
    }

    [[nodiscard]] std::string report_payload_to_string(
        const J::token &body)
    {
      if (!body.is_object())
      {
        return {};
      }

      if (body.contains("report"))
      {
        const auto &report = body["report"];

        if (report.is_string())
        {
          return report.as_string_or("");
        }

        try
        {
          return report.dump();
        }
        catch (...)
        {
          return {};
        }
      }

      if (body.contains("raw_json"))
      {
        const auto &raw_json = body["raw_json"];

        if (raw_json.is_string())
        {
          return raw_json.as_string_or("");
        }

        try
        {
          return raw_json.dump();
        }
        catch (...)
        {
          return {};
        }
      }

      return {};
    }

    [[nodiscard]] J::token report_to_json(
        const Report &report)
    {
      return J::obj({
          "id",
          report.public_id,
          "run_id",
          report.run_public_id,
          "summary",
          report.summary,
          "raw_json",
          report.raw_json,
          "created_at",
          report.created_at,
          "updated_at",
          report.updated_at,
      });
    }

    [[nodiscard]] J::token reports_to_json(
        const std::vector<Report> &reports)
    {
      std::vector<J::token> items;
      items.reserve(reports.size());

      for (const auto &report : reports)
      {
        items.push_back(report_to_json(report));
      }

      return J::array(std::move(items));
    }

    void respond_report_result(
        vix::Response &res,
        const ReportResult &result)
    {
      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          report_to_json(result.report));
    }

    void respond_report_list_result(
        vix::Response &res,
        const ReportListResult &result)
    {
      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          J::obj({
              "count",
              static_cast<long long>(result.reports.size()),
              "total",
              result.total,
              "limit",
              result.limit,
              "offset",
              result.offset,
              "items",
              reports_to_json(result.reports),
          }));
    }
  }

  void ReportController::submit_for_run(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      const auto &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      const std::string summary =
          string_field_or_empty(body, "summary");

      const std::string raw_json =
          report_payload_to_string(body);

      ReportService service{state};

      const auto result =
          service.submit_report(
              run_id,
              summary,
              raw_json);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      if (result.status == 201)
      {
        softadastra::cloud::http::JsonResponse::created(
            res,
            report_to_json(result.report));

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          report_to_json(result.report));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "submit report failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ReportController::list(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const int limit =
          std::clamp(
              int_query_or(req, "limit", 20),
              1,
              100);

      const int offset =
          std::max(
              int_query_or(req, "offset", 0),
              0);

      ReportService service{state};

      const auto result =
          service.list_reports(
              limit,
              offset);

      respond_report_list_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "list reports failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ReportController::get(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string report_id =
          req.param("id", "");

      ReportService service{state};

      const auto result =
          service.get_report(report_id);

      respond_report_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "get report failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ReportController::get_for_run(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      ReportService service{state};

      const auto result =
          service.get_report_for_run(run_id);

      respond_report_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "get run report failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ReportController::remove(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string report_id =
          req.param("id", "");

      ReportService service{state};

      const auto result =
          service.delete_report(report_id);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          J::obj({
              "id",
              result.report.public_id,
              "deleted",
              true,
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "delete report failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ReportController::remove_for_run(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      ReportService service{state};

      const auto result =
          service.delete_report_for_run(run_id);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          J::obj({
              "run_id",
              result.report.run_public_id,
              "report_id",
              result.report.public_id,
              "deleted",
              true,
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "delete run report failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

} // namespace softadastra::cloud::modules::reports
