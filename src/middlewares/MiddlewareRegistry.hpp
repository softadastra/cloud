/**
 *
 *  @file MiddlewareRegistry.hpp
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

#ifndef SOFTADASTRA_CLOUD_MIDDLEWARES_MIDDLEWARE_REGISTRY_HPP
#define SOFTADASTRA_CLOUD_MIDDLEWARES_MIDDLEWARE_REGISTRY_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::middlewares
{
  /**
   * @brief Central middleware installer for Softadastra Cloud.
   *
   * MiddlewareRegistry installs all global and prefix-based middlewares used
   * by the backend.
   *
   * It keeps AppFactory clean and makes middleware order explicit.
   */
  class MiddlewareRegistry final
  {
  public:
    /**
     * @brief Installs all middlewares.
     *
     * @param app Vix application.
     * @param state Shared application state.
     */
    static void install(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

  private:
    MiddlewareRegistry() = delete;

    static void install_global_middlewares(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

    static void install_agent_middlewares(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);
  };

} // namespace softadastra::cloud::middlewares

#endif // SOFTADASTRA_CLOUD_MIDDLEWARES_MIDDLEWARE_REGISTRY_HPP
