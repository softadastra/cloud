/**
 *
 *  @file EventRoutes.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_ROUTES_HPP
#define SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_ROUTES_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::events
{
  /**
   * @brief Registers run event routes.
   *
   * EventRoutes only declares HTTP paths and forwards requests to
   * EventController.
   */
  class EventRoutes final
  {
  public:
    /**
     * @brief Registers event routes on the Vix app.
     *
     * @param app Vix application.
     * @param state Shared application state.
     */
    static void register_routes(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

  private:
    EventRoutes() = delete;
  };

} // namespace softadastra::cloud::modules::events

#endif // SOFTADASTRA_CLOUD_MODULES_EVENTS_EVENT_ROUTES_HPP
