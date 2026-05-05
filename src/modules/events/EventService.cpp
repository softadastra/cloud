/*
 * EventService.cpp
 */

#include "modules/events/EventService.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "utils/IdGenerator.hpp"
#include "utils/Time.hpp"

namespace softadastra::cloud::modules::events
{
  namespace
  {
    constexpr std::size_t MIN_EVENT_TYPE_LENGTH = 2;
    constexpr std::size_t MAX_EVENT_TYPE_LENGTH = 120;
    constexpr std::size_t MAX_MESSAGE_LENGTH = 1000;
    constexpr std::size_t MAX_PAYLOAD_LENGTH = 1024 * 1024;
  }

  EventService::EventService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        repository_(state.database),
        run_repository_(state.database)
  {
  }

  EventResult EventService::create_event(
      const std::string &run_public_id,
      const std::string &event_type,
      const std::string &severity,
      const std::string &message,
      const std::string &payload)
  {
    if (run_public_id.empty())
    {
      return validation_error("Run id is required");
    }

    if (!is_valid_event_type(event_type))
    {
      return validation_error(
          "Event type must contain between 2 and 120 characters");
    }

    const std::string normalized_severity =
        normalize_severity(severity.empty() ? "info" : severity);

    if (!is_valid_severity(normalized_severity))
    {
      return validation_error(
          "Severity must be info, warning, error, or critical");
    }

    if (!is_valid_message(message))
    {
      return validation_error(
          "Event message must contain at most 1000 characters");
    }

    if (!is_valid_payload(payload))
    {
      return validation_error(
          "Event payload must contain at most 1MB");
    }

    const auto run =
        run_repository_.find_by_public_id(run_public_id);

    if (!run.has_value())
    {
      return not_found("Run not found");
    }

    const std::string public_id =
        softadastra::cloud::utils::IdGenerator::generate("evt");

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool created =
        repository_.create(
            public_id,
            run->id,
            event_type,
            normalized_severity,
            message,
            payload,
            now);

    if (!created)
    {
      return internal_error("Failed to create event");
    }

    auto event =
        repository_.find_by_public_id(public_id);

    if (!event.has_value())
    {
      return internal_error("Event was created but could not be loaded");
    }

    EventResult result;
    result.success = true;
    result.status = 201;
    result.error.clear();
    result.message = "Event created";
    result.event = *event;

    return result;
  }

  EventListResult EventService::list_events(
      int limit,
      int offset)
  {
    EventListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Events loaded";
    result.limit = std::clamp(limit, 1, 500);
    result.offset = std::max(offset, 0);
    result.events =
        repository_.all(
            result.limit,
            result.offset);
    result.total = repository_.count();

    return result;
  }

  EventListResult EventService::list_events_for_run(
      const std::string &run_public_id,
      int limit,
      int offset)
  {
    if (run_public_id.empty())
    {
      return list_error(
          400,
          "validation_failed",
          "Run id is required");
    }

    const auto run =
        run_repository_.find_by_public_id(run_public_id);

    if (!run.has_value())
    {
      return list_error(
          404,
          "run_not_found",
          "Run not found");
    }

    EventListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Events loaded";
    result.limit = std::clamp(limit, 1, 500);
    result.offset = std::max(offset, 0);
    result.events =
        repository_.all_for_run(
            run_public_id,
            result.limit,
            result.offset);
    result.total =
        repository_.count_for_run(run_public_id);

    return result;
  }

  EventResult EventService::get_event(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Event id is required");
    }

    auto event =
        repository_.find_by_public_id(public_id);

    if (!event.has_value())
    {
      return not_found("Event not found");
    }

    EventResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Event loaded";
    result.event = *event;

    return result;
  }

  std::int64_t EventService::count_by_severity_for_run(
      const std::string &run_public_id,
      const std::string &severity) const
  {
    if (run_public_id.empty() || severity.empty())
    {
      return 0;
    }

    return repository_.count_by_severity_for_run(
        run_public_id,
        normalize_severity(severity));
  }

  std::int64_t EventService::count_by_type_for_run(
      const std::string &run_public_id,
      const std::string &event_type) const
  {
    if (run_public_id.empty() || event_type.empty())
    {
      return 0;
    }

    return repository_.count_by_type_for_run(
        run_public_id,
        event_type);
  }

  bool EventService::is_valid_event_type(
      const std::string &event_type) noexcept
  {
    return event_type.size() >= MIN_EVENT_TYPE_LENGTH &&
           event_type.size() <= MAX_EVENT_TYPE_LENGTH;
  }

  bool EventService::is_valid_severity(
      const std::string &severity) noexcept
  {
    return severity == "info" ||
           severity == "warning" ||
           severity == "warn" ||
           severity == "error" ||
           severity == "critical";
  }

  bool EventService::is_valid_message(
      const std::string &message) noexcept
  {
    return message.size() <= MAX_MESSAGE_LENGTH;
  }

  bool EventService::is_valid_payload(
      const std::string &payload) noexcept
  {
    return payload.size() <= MAX_PAYLOAD_LENGTH;
  }

  std::string EventService::normalize_severity(
      const std::string &severity)
  {
    std::string normalized = severity;

    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c)
        {
          return static_cast<char>(std::tolower(c));
        });

    if (normalized == "warn")
    {
      return "warning";
    }

    return normalized;
  }

  EventResult EventService::validation_error(
      std::string message)
  {
    EventResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  EventResult EventService::not_found(
      std::string message)
  {
    EventResult result;
    result.success = false;
    result.status = 404;
    result.error = "event_not_found";
    result.message = std::move(message);
    return result;
  }

  EventResult EventService::internal_error(
      std::string message)
  {
    EventResult result;
    result.success = false;
    result.status = 500;
    result.error = "internal_error";
    result.message = std::move(message);
    return result;
  }

  EventListResult EventService::list_error(
      int status,
      std::string error,
      std::string message)
  {
    EventListResult result;
    result.success = false;
    result.status = status;
    result.error = std::move(error);
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::events
