/*
 * RunValidator.cpp
 */

#include "validation/RunValidator.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace softadastra::cloud::validation
{
  namespace
  {
    constexpr std::size_t MAX_TARGET_URL_LENGTH = 2048;
    constexpr std::size_t MIN_SCENARIO_LENGTH = 2;
    constexpr std::size_t MAX_SCENARIO_LENGTH = 120;
  }

  std::optional<ValidationError> RunValidator::validate_target_url(
      const std::string &target_url)
  {
    if (target_url.empty())
    {
      return ValidationError{
          "target_url",
          "required",
          "Target URL is required"};
    }

    if (target_url.size() > MAX_TARGET_URL_LENGTH)
    {
      return ValidationError{
          "target_url",
          "too_long",
          "Target URL must contain at most 2048 characters"};
    }

    if (!starts_with(target_url, "http://") &&
        !starts_with(target_url, "https://"))
    {
      return ValidationError{
          "target_url",
          "invalid_url",
          "Target URL must start with http:// or https://"};
    }

    return std::nullopt;
  }

  std::optional<ValidationError> RunValidator::validate_method(
      const std::string &method)
  {
    const std::string normalized =
        normalize_method(method.empty() ? "GET" : method);

    if (normalized == "GET" ||
        normalized == "POST" ||
        normalized == "PUT" ||
        normalized == "PATCH" ||
        normalized == "DELETE" ||
        normalized == "HEAD" ||
        normalized == "OPTIONS")
    {
      return std::nullopt;
    }

    return ValidationError{
        "method",
        "invalid_method",
        "HTTP method must be GET, POST, PUT, PATCH, DELETE, HEAD, or OPTIONS"};
  }

  std::optional<ValidationError> RunValidator::validate_scenario(
      const std::string &scenario)
  {
    if (scenario.empty())
    {
      return ValidationError{
          "scenario",
          "required",
          "Scenario is required"};
    }

    if (scenario.size() < MIN_SCENARIO_LENGTH ||
        scenario.size() > MAX_SCENARIO_LENGTH)
    {
      return ValidationError{
          "scenario",
          "invalid_length",
          "Scenario must contain between 2 and 120 characters"};
    }

    return std::nullopt;
  }

  std::optional<ValidationError> RunValidator::validate_status(
      const std::string &status)
  {
    if (status == "created" ||
        status == "running" ||
        status == "finished" ||
        status == "failed" ||
        status == "cancelled")
    {
      return std::nullopt;
    }

    return ValidationError{
        "status",
        "invalid_status",
        "Status must be created, running, finished, failed, or cancelled"};
  }

  bool RunValidator::is_final_status(
      const std::string &status) noexcept
  {
    return status == "finished" ||
           status == "failed" ||
           status == "cancelled";
  }

  std::string RunValidator::normalize_method(
      const std::string &method)
  {
    std::string normalized = method;

    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c)
        {
          return static_cast<char>(std::toupper(c));
        });

    return normalized;
  }

  bool RunValidator::starts_with(
      const std::string &value,
      const std::string &prefix) noexcept
  {
    return value.rfind(prefix, 0) == 0;
  }

} // namespace softadastra::cloud::validation
