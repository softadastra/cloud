/**
 *
 *  @file EventRepository.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_REPOSITORY_HPP
#define SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/db/db.hpp>

#include "modules/events/Event.hpp"

namespace softadastra::cloud::modules::events
{
  /**
   * @brief Repository for run event persistence.
   *
   * EventRepository owns all SQL access for run_events.
   *
   * It does not contain HTTP logic or business rules. Those belong to
   * EventController and EventService.
   */
  class EventRepository final
  {
  public:
    /**
     * @brief Creates an event repository.
     *
     * @param database Shared Vix database instance.
     */
    explicit EventRepository(
        std::shared_ptr<vix::db::Database> database);

    /**
     * @brief Creates a new event for a run.
     *
     * @param public_id Public event id.
     * @param run_id Internal run id.
     * @param event_type Event type.
     * @param severity Event severity.
     * @param message Human-readable event message.
     * @param payload Raw JSON payload string.
     * @param created_at Creation timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool create(
        const std::string &public_id,
        std::int64_t run_id,
        const std::string &event_type,
        const std::string &severity,
        const std::string &message,
        const std::string &payload,
        std::int64_t created_at);

    /**
     * @brief Returns all events.
     *
     * @param limit Max number of events.
     * @param offset Offset for pagination.
     * @return Event list.
     */
    [[nodiscard]] std::vector<Event> all(
        int limit,
        int offset) const;

    /**
     * @brief Returns all events for a run.
     *
     * @param run_public_id Public run id.
     * @param limit Max number of events.
     * @param offset Offset for pagination.
     * @return Event list.
     */
    [[nodiscard]] std::vector<Event> all_for_run(
        const std::string &run_public_id,
        int limit,
        int offset) const;

    /**
     * @brief Finds one event by internal id.
     *
     * @param id Internal database id.
     * @return Event if found.
     */
    [[nodiscard]] std::optional<Event> find_by_id(
        std::int64_t id) const;

    /**
     * @brief Finds one event by public id.
     *
     * @param public_id Public event id.
     * @return Event if found.
     */
    [[nodiscard]] std::optional<Event> find_by_public_id(
        const std::string &public_id) const;

    /**
     * @brief Counts all events.
     *
     * @return Event count.
     */
    [[nodiscard]] std::int64_t count() const;

    /**
     * @brief Counts all events for one run.
     *
     * @param run_public_id Public run id.
     * @return Event count.
     */
    [[nodiscard]] std::int64_t count_for_run(
        const std::string &run_public_id) const;

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

    /**
     * @brief Deletes all events for one run.
     *
     * Useful when deleting a run.
     *
     * @param run_public_id Public run id.
     * @return true on success.
     */
    [[nodiscard]] bool remove_for_run(
        const std::string &run_public_id);

  private:
    std::shared_ptr<vix::db::Database> database_;

    [[nodiscard]] static Event event_from_current_row(
        const vix::db::ResultRow &row);

    [[nodiscard]] static int normalize_limit(int limit) noexcept;
    [[nodiscard]] static int normalize_offset(int offset) noexcept;
  };

} // namespace softadastra::cloud::modules::events

#endif // SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_REPOSITORY_HPP
