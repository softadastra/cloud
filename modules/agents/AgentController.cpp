/*
 * AgentController.cpp
 */

#include "modules/agents/AgentController.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include <vix/log/Log.hpp>

#include "http/JsonResponse.hpp"
#include "http/RequestContext.hpp"
#include "modules/agents/AgentService.hpp"

namespace softadastra::cloud::modules::agents
{
  namespace J = vix::json;

  namespace
  {
    constexpr const char *AGENT_API_KEY_HEADER = "x-agent-api-key";

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

    [[nodiscard]] J::token agent_to_json(
        const Agent &agent)
    {
      return J::obj({
          "id",
          agent.public_id,
          "project_id",
          agent.project_public_id,
          "name",
          agent.name,
          "last_seen_at",
          agent.last_seen_at,
          "created_at",
          agent.created_at,
          "updated_at",
          agent.updated_at,
      });
    }

    [[nodiscard]] J::token agents_to_json(
        const std::vector<Agent> &agents)
    {
      std::vector<J::token> items;
      items.reserve(agents.size());

      for (const auto &agent : agents)
      {
        items.push_back(agent_to_json(agent));
      }

      return J::array(std::move(items));
    }

    [[nodiscard]] J::token agent_with_key_to_json(
        const AgentResult &result)
    {
      return J::obj({
          "agent",
          agent_to_json(result.agent),
          "api_key",
          result.api_key,
          "api_key_warning",
          "Store this key now. It will not be shown again.",
      });
    }

    void respond_agent_result(
        vix::Response &res,
        const AgentResult &result)
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

      if (!result.api_key.empty())
      {
        softadastra::cloud::http::JsonResponse::data(
            res,
            result.message,
            agent_with_key_to_json(result));

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          agent_to_json(result.agent));
    }

    void attach_agent_context(
        vix::Request &req,
        const Agent &agent)
    {
      auto *context =
          req.try_state<softadastra::cloud::http::RequestContext>();

      if (context == nullptr)
      {
        softadastra::cloud::http::RequestContext fresh_context;
        fresh_context.set_agent_auth(
            agent.public_id,
            agent.project_public_id);

        req.emplace_state<softadastra::cloud::http::RequestContext>(
            fresh_context);

        return;
      }

      context->set_agent_auth(
          agent.public_id,
          agent.project_public_id);
    }
  }

  void AgentController::create(
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

      const std::string project_id =
          string_field_or_empty(body, "project_id");

      const std::string name =
          string_field_or_empty(body, "name");

      AgentService service{state};

      const auto result =
          service.create_agent(
              project_id,
              name);

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
          agent_with_key_to_json(result));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "create agent failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AgentController::list(
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

      AgentService service{state};

      const auto result =
          project_id.empty()
              ? service.list_agents(limit, offset)
              : service.list_agents_for_project(project_id, limit, offset);

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
              static_cast<long long>(result.agents.size()),
              "total",
              result.total,
              "limit",
              result.limit,
              "offset",
              result.offset,
              "items",
              agents_to_json(result.agents),
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "list agents failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AgentController::get(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string agent_id =
          req.param("id", "");

      AgentService service{state};

      const auto result =
          service.get_agent(agent_id);

      respond_agent_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "get agent failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AgentController::rotate_key(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string agent_id =
          req.param("id", "");

      AgentService service{state};

      const auto result =
          service.rotate_api_key(agent_id);

      respond_agent_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "rotate agent api key failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AgentController::remove(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string agent_id =
          req.param("id", "");

      AgentService service{state};

      const auto result =
          service.delete_agent(agent_id);

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
              result.agent.public_id,
              "deleted",
              true,
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "delete agent failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AgentController::verify(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string api_key =
          req.header(AGENT_API_KEY_HEADER);

      AgentService service{state};

      const auto result =
          service.authenticate_api_key(api_key);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      attach_agent_context(
          req,
          result.agent);

      softadastra::cloud::http::JsonResponse::data(
          res,
          "Agent authenticated",
          J::obj({
              "authenticated",
              true,
              "agent",
              agent_to_json(result.agent),
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "verify agent failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

} // namespace softadastra::cloud::modules::agents
