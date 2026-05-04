/**
 *
 *  @file HealthRoutes.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_HEALTH_HEALTH_ROUTES_HPP
#define SOFTADASTRA_CLOUD_MODULES_HEALTH_HEALTH_ROUTES_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::health
{
  /**
   * @brief Registers health and readiness routes.
   *
   * HealthRoutes keeps route declaration separate from controller logic.
   */
  class HealthRoutes final
  {
  public:
    /**
     * @brief Registers health routes on the Vix app.
     *
     * @param app Vix application.
     * @param state Shared application state.
     */
    static void register_routes(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

  private:
    HealthRoutes() = delete;
  };

} // namespace softadastra::cloud::modules::health

#endif // SOFTADASTRA_CLOUD_MODULES_HEALTH_HEALTH_ROUTES_HPP
