/**
 *
 *  @file EventController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::events
{
  /**
   * @brief HTTP controller for run event endpoints.
   *
   * EventController handles only HTTP concerns:
   * - read path/query parameters
   * - read JSON request body
   * - call EventService
   * - return standardized JSON responses
   *
   * Business rules belong in EventService.
   * SQL belongs in EventRepository.
   */
  class EventController final
  {
  public:
    /**
     * @brief Creates a new event for a run.
     *
     * Path:
     * /api/runs/{id}/events
     *
     * Expected JSON body:
     * {
     *   "event_type": "request_timeout",
     *   "severity": "warning",
     *   "message": "Request timed out after 5000ms",
     *   "payload": "{...}"
     * }
     */
    static void create(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Lists events.
     *
     * Query params:
     * - limit
     * - offset
     * - run_id optional
     */
    static void list(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Lists events for a run.
     *
     * Path:
     * /api/runs/{id}/events
     */
    static void list_for_run(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Gets one event by public id.
     *
     * Path:
     * /api/events/{id}
     */
    static void get(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    EventController() = delete;
  };

} // namespace softadastra::cloud::modules::events

#endif // SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_CONTROLLER_HPP
