/*
 * EventController.cpp
 */

#include "modules/events/EventController.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include <vix/json/convert.hpp>
#include <vix/json/json.hpp>
#include <vix/log/Log.hpp>

#include "http/JsonResponse.hpp"
#include "modules/events/EventService.hpp"

namespace softadastra::cloud::modules::events
{
  namespace J = vix::json;

  namespace
  {
    [[nodiscard]] bool has_string_field(
        const J::Json &body,
        std::string_view field)
    {
      return body.is_object() &&
             body.contains(std::string(field)) &&
             body.at(std::string(field)).is_string();
    }

    [[nodiscard]] std::string string_field_or_empty(
        const J::Json &body,
        std::string_view field)
    {
      return J::get_or<std::string>(
          body,
          field,
          "");
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

    [[nodiscard]] std::string payload_to_string(
        const J::Json &body)
    {
      if (!body.is_object())
      {
        return {};
      }

      const J::Json *payload = J::ptr(body, "payload");

      if (payload == nullptr)
      {
        return {};
      }

      if (payload->is_string())
      {
        return payload->get<std::string>();
      }

      return payload->dump();
    }

    [[nodiscard]] J::OrderedJson event_to_json(
        const Event &event)
    {
      return J::o(
          "id",
          event.public_id,
          "run_id",
          event.run_public_id,
          "event_type",
          event.event_type,
          "severity",
          event.severity,
          "message",
          event.message,
          "payload",
          event.payload,
          "created_at",
          event.created_at);
    }

    [[nodiscard]] J::Json events_to_json(
        const std::vector<Event> &events)
    {
      J::Json items = J::Json::array();

      for (const auto &event : events)
      {
        items.push_back(J::Json(event_to_json(event)));
      }

      return items;
    }

    void respond_event_result(
        vix::Response &res,
        const EventResult &result)
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
          event_to_json(result.event));
    }

    void respond_event_list_result(
        vix::Response &res,
        const EventListResult &result)
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
          J::o(
              "count",
              static_cast<long long>(result.events.size()),
              "total",
              result.total,
              "limit",
              result.limit,
              "offset",
              result.offset,
              "items",
              events_to_json(result.events)));
    }
  }

  void EventController::create(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      const J::Json &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      const std::string event_type =
          string_field_or_empty(body, "event_type");

      const std::string severity =
          string_field_or_empty(body, "severity");

      const std::string message =
          string_field_or_empty(body, "message");

      const std::string payload =
          payload_to_string(body);

      EventService service{state};

      const auto result =
          service.create_event(
              run_id,
              event_type,
              severity,
              message,
              payload);

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
          event_to_json(result.event));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "create event failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void EventController::list(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const int limit =
          std::clamp(
              int_query_or(req, "limit", 50),
              1,
              500);

      const int offset =
          std::max(
              int_query_or(req, "offset", 0),
              0);

      const std::string run_id =
          req.query_value("run_id", "");

      EventService service{state};

      const auto result =
          run_id.empty()
              ? service.list_events(limit, offset)
              : service.list_events_for_run(run_id, limit, offset);

      respond_event_list_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "list events failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void EventController::list_for_run(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string run_id =
          req.param("id", "");

      const int limit =
          std::clamp(
              int_query_or(req, "limit", 50),
              1,
              500);

      const int offset =
          std::max(
              int_query_or(req, "offset", 0),
              0);

      EventService service{state};

      const auto result =
          service.list_events_for_run(
              run_id,
              limit,
              offset);

      respond_event_list_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "list run events failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void EventController::get(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string event_id =
          req.param("id", "");

      EventService service{state};

      const auto result =
          service.get_event(event_id);

      respond_event_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "get event failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

} // namespace softadastra::cloud::modules::events
