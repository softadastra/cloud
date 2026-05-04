/**
 *
 *  @file HealthController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_HEALTH_HEALTH_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_HEALTH_HEALTH_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::health
{
  /**
   * @brief HTTP controller for health endpoints.
   *
   * HealthController exposes lightweight endpoints used by developers,
   * load balancers, uptime checks, and deployment tooling.
   */
  class HealthController final
  {
  public:
    /**
     * @brief Returns a basic service health response.
     *
     * @param req Vix HTTP request.
     * @param res Vix HTTP response.
     * @param state Shared application state.
     */
    static void health(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Returns a database-aware health response.
     *
     * @param req Vix HTTP request.
     * @param res Vix HTTP response.
     * @param state Shared application state.
     */
    static void readiness(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    HealthController() = delete;
  };

} // namespace softadastra::cloud::modules::health

#endif // SOFTADASTRA_CLOUD_MODULES_HEALTH_HEALTH_CONTROLLER_HPP
