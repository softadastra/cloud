/**
 *
 *  @file AuthRoutes.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_ROUTES_HPP
#define SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_ROUTES_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::auth
{
  /**
   * @brief Registers dashboard authentication routes.
   *
   * AuthRoutes only declares HTTP paths and forwards the work to
   * AuthController.
   */
  class AuthRoutes final
  {
  public:
    /**
     * @brief Registers authentication routes on the Vix app.
     *
     * @param app Vix application.
     * @param state Shared application state.
     */
    static void register_routes(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

  private:
    AuthRoutes() = delete;
  };

} // namespace softadastra::cloud::modules::auth

#endif // SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_ROUTES_HPP
