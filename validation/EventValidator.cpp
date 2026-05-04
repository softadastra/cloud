/*
 * EventValidator.cpp
 */

#include "validation/EventValidator.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace softadastra::cloud::validation
{
  namespace
  {
    constexpr std::size_t MIN_EVENT_TYPE_LENGTH = 2;
    constexpr std::size_t MAX_EVENT_TYPE_LENGTH = 120;
    constexpr std::size_t MAX_MESSAGE_LENGTH = 1000;
    constexpr std::size_t MAX_PAYLOAD_LENGTH = 1024 * 1024;
  }

  std::optional<ValidationError> EventValidator::validate_event_type(
      const std::string &event_type)
  {
    if (event_type.empty())
    {
      return ValidationError{
          "event_type",
          "required",
          "Event type is required"};
    }

    if (event_type.size() < MIN_EVENT_TYPE_LENGTH ||
        event_type.size() > MAX_EVENT_TYPE_LENGTH)
    {
      return ValidationError{
          "event_type",
          "invalid_length",
          "Event type must contain between 2 and 120 characters"};
    }

    return std::nullopt;
  }

  std::optional<ValidationError> EventValidator::validate_severity(
      const std::string &severity)
  {
    const std::string normalized =
        normalize_severity(severity.empty() ? "info" : severity);

    if (normalized == "info" ||
        normalized == "warning" ||
        normalized == "error" ||
        normalized == "critical")
    {
      return std::nullopt;
    }

    return ValidationError{
        "severity",
        "invalid_severity",
        "Severity must be info, warning, error, or critical"};
  }

  std::optional<ValidationError> EventValidator::validate_message(
      const std::string &message)
  {
    if (message.size() > MAX_MESSAGE_LENGTH)
    {
      return ValidationError{
          "message",
          "too_long",
          "Event message must contain at most 1000 characters"};
    }

    return std::nullopt;
  }

  std::optional<ValidationError> EventValidator::validate_payload(
      const std::string &payload)
  {
    if (payload.size() > MAX_PAYLOAD_LENGTH)
    {
      return ValidationError{
          "payload",
          "too_large",
          "Event payload must contain at most 1MB"};
    }

    return std::nullopt;
  }

  std::string EventValidator::normalize_severity(
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

    if (normalized.empty())
    {
      return "info";
    }

    if (normalized == "warn")
    {
      return "warning";
    }

    return normalized;
  }

} // namespace softadastra::cloud::validation
