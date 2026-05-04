/*
 * RunController.cpp
 */

#include "modules/runs/RunController.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include <vix/log/Log.hpp>

#include "http/JsonResponse.hpp"
#include "http/RequestContext.hpp"
#include "modules/runs/RunService.hpp"

namespace softadastra::cloud::modules::runs
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

    [[nodiscard]] std::string request_agent_id(
        const vix::Request &req)
    {
      const auto *context =
          req.try_state<softadastra::cloud::http::RequestContext>();

      if (context == nullptr)
      {
        return "";
      }

      return context->agent_public_id;
    }

    [[nodiscard]] std::string request_project_id(
        const vix::Request &req)
    {
      const auto *context =
          req.try_state<softadastra::cloud::http::RequestContext>();

      if (context == nullptr)
      {
        return "";
      }

      return context->project_public_id;
    }

    [[nodiscard]] J::token run_to_json(
        const Run &run)
    {
      return J::obj({
          "id",
          run.public_id,
          "project_id",
          run.project_public_id,
          "agent_id",
          run.agent_public_id,
          "status",
          run.status,
          "target_url",
          run.target_url,
          "method",
          run.method,
          "scenario",
          run.scenario,
          "started_at",
          run.started_at,
          "finished_at",
          run.finished_at,
          "created_at",
          run.created_at,
          "updated_at",
          run.updated_at,
      });
    }

    [[nodiscard]] J::token runs_to_json(
        const std::vector<Run> &runs)
    {
      std::vector<J::token> items;
      items.reserve(runs.size());

      for (const auto &run : runs)
      {
        items.push_back(run_to_json(run));
      }

      return J::array(std::move(items));
    }

    void respond_run_result(
        vix::Response &res,
        const RunResult &result)
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
          run_to_json(result.run));
    }
  }

  void RunController::create(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const auto &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      std::string project_id =
          string_field_or_empty(body, "project_id");

      std::string agent_id =
          string_field_or_empty(body, "agent_id");

      if (project_id.empty())
      {
        project_id = request_project_id(req);
      }

      if (agent_id.empty())
      {
        agent_id = request_agent_id(req);
      }

      const std::string target_url =
          string_field_or_empty(body, "target_url");

      const std::string method =
          string_field_or_empty(body, "method");

      const std::string scenario =
          string_field_or_empty(body, "scenario");

      RunService service{state};

      const auto result =
          service.create_run(
              project_id,
              agent_id,
              target_url,
              method,
              scenario);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::created(
          res,
          run_to_json(result.run));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "create run failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void RunController::list(
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

      const std::string project_id =
          req.query_value("project_id", "");

      const std::string agent_id =
          req.query_value("agent_id", "");

      RunService service{state};

      RunListResult result;

      if (!project_id.empty())
      {
        result =
            service.list_runs_for_project(
                project_id,
                limit,
                offset);
      }
      else if (!agent_id.empty())
      {
        result =
            service.list_runs_for_agent(
                agent_id,
                limit,
                offset);
      }
      else
      {
        result =
            service.list_runs(
                limit,
                offset);
      }

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
              static_cast<long long>(result.runs.size()),
              "total",
              result.total,
              "limit",
              result.limit,
              "offset",
              result.offset,
              "items",
              runs_to_json(result.runs),
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "list runs failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void RunController::get(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      RunService service{state};

      const auto result =
          service.get_run(run_id);

      respond_run_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "get run failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void RunController::update_status(
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

      const std::string status =
          string_field_or_empty(body, "status");

      RunService service{state};

      const auto result =
          service.update_status(
              run_id,
              status);

      respond_run_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "update run status failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void RunController::finish(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      const auto &body = req.json();

      std::string status = "finished";

      if (body.is_object())
      {
        const std::string provided_status =
            string_field_or_empty(body, "status");

        if (!provided_status.empty())
        {
          status = provided_status;
        }
      }

      RunService service{state};

      const auto result =
          service.finish_run(
              run_id,
              status);

      respond_run_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "finish run failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void RunController::remove(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      RunService service{state};

      const auto result =
          service.delete_run(run_id);

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
              result.run.public_id,
              "deleted",
              true,
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "delete run failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

} // namespace softadastra::cloud::modules::runs
