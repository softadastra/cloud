/**
 *
 *  @file EventService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_SERVICE_HPP

#include <string>
#include <vector>

#include "app/AppState.hpp"
#include "modules/events/Event.hpp"
#include "modules/events/EventRepository.hpp"
#include "modules/runs/RunRepository.hpp"

namespace softadastra::cloud::modules::events
{
  /**
   * @brief Result returned by event operations.
   */
  struct EventResult
  {
    bool success{false};
    int status{400};

    std::string error{"event_error"};
    std::string message{};

    Event event{};
  };

  /**
   * @brief List result returned by event listing.
   */
  struct EventListResult
  {
    bool success{false};
    int status{400};

    std::string error{"event_error"};
    std::string message{};

    std::vector<Event> events{};
    std::int64_t total{0};
    int limit{50};
    int offset{0};
  };

  /**
   * @brief Event business service.
   *
   * EventService owns the business rules for run events:
   * - validating event payloads
   * - checking run existence
   * - creating events
   * - listing events by run
   * - exposing event counters for later scoring
   *
   * It does not read Vix Request and does not write Vix Response.
   */
  class EventService final
  {
  public:
    /**
     * @brief Creates an event service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit EventService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Creates a new event for a run.
     *
     * @param run_public_id Public run id.
     * @param event_type Event type.
     * @param severity Event severity.
     * @param message Human-readable message.
     * @param payload Raw JSON payload string.
     * @return Event result.
     */
    [[nodiscard]] EventResult create_event(
        const std::string &run_public_id,
        const std::string &event_type,
        const std::string &severity,
        const std::string &message,
        const std::string &payload);

    /**
     * @brief Lists all events.
     *
     * @param limit Max number of events.
     * @param offset Pagination offset.
     * @return Event list result.
     */
    [[nodiscard]] EventListResult list_events(
        int limit,
        int offset);

    /**
     * @brief Lists all events for one run.
     *
     * @param run_public_id Public run id.
     * @param limit Max number of events.
     * @param offset Pagination offset.
     * @return Event list result.
     */
    [[nodiscard]] EventListResult list_events_for_run(
        const std::string &run_public_id,
        int limit,
        int offset);

    /**
     * @brief Gets one event by public id.
     *
     * @param public_id Public event id.
     * @return Event result.
     */
    [[nodiscard]] EventResult get_event(
        const std::string &public_id);

    /**
     * @brief Counts events by severity for one run.
     *
     * @param run_public_id Public run id.
     * @param severity Event severity.
     * @return Matching event count.
     */
    [[nodiscard]] std::int64_t count_by_severity_for_run(
        const std::string &run_public_id,
        const std::string &severity) const;

    /**
     * @brief Counts events by type for one run.
     *
     * @param run_public_id Public run id.
     * @param event_type Event type.
     * @return Matching event count.
     */
    [[nodiscard]] std::int64_t count_by_type_for_run(
        const std::string &run_public_id,
        const std::string &event_type) const;

  private:
    const softadastra::cloud::app::AppState &state_;
    EventRepository repository_;
    softadastra::cloud::modules::runs::RunRepository run_repository_;

    [[nodiscard]] static bool is_valid_event_type(
        const std::string &event_type) noexcept;

    [[nodiscard]] static bool is_valid_severity(
        const std::string &severity) noexcept;

    [[nodiscard]] static bool is_valid_message(
        const std::string &message) noexcept;

    [[nodiscard]] static bool is_valid_payload(
        const std::string &payload) noexcept;

    [[nodiscard]] static std::string normalize_severity(
        const std::string &severity);

    [[nodiscard]] static EventResult validation_error(
        std::string message);

    [[nodiscard]] static EventResult not_found(
        std::string message);

    [[nodiscard]] static EventResult internal_error(
        std::string message);

    [[nodiscard]] static EventListResult list_error(
        int status,
        std::string error,
        std::string message);
  };

} // namespace softadastra::cloud::modules::events

#endif // SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_SERVICE_HPP
