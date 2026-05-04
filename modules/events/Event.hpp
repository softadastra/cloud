/**
 *
 *  @file Event.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra/cloud
 *
 *  Licensed under the Apache License, Version 2.0.
 *
 *  Softadastra Cloud
 *
 */

#ifndef SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_HPP
#define SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::modules::events
{
  /**
   * @brief Event model for a reliability test run.
   *
   * A run event represents one observable step produced by a Softadastra agent
   * during a reliability test.
   *
   * Examples:
   * - run_started
   * - request_started
   * - request_timeout
   * - retry_started
   * - response_received
   * - duplicate_detected
   * - data_loss_risk_detected
   * - run_finished
   */
  struct Event
  {
    std::int64_t id{0};
    std::string public_id{};

    std::int64_t run_id{0};
    std::string run_public_id{};

    std::string event_type{};
    std::string severity{"info"};
    std::string message{};
    std::string payload{};

    std::int64_t created_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return id > 0 &&
             !public_id.empty() &&
             run_id > 0 &&
             !event_type.empty() &&
             !severity.empty();
    }

    [[nodiscard]] bool has_run() const noexcept
    {
      return run_id > 0 || !run_public_id.empty();
    }

    [[nodiscard]] bool has_payload() const noexcept
    {
      return !payload.empty();
    }

    [[nodiscard]] bool is_error() const noexcept
    {
      return severity == "error" ||
             severity == "critical";
    }

    [[nodiscard]] bool is_warning() const noexcept
    {
      return severity == "warning" ||
             severity == "warn";
    }
  };

} // namespace softadastra::cloud::modules::events

#endif // SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_HPP
