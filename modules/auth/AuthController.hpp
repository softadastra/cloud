/**
 *
 *  @file AuthController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::auth
{
  /**
   * @brief HTTP controller for dashboard authentication endpoints.
   *
   * AuthController is responsible only for HTTP concerns:
   * - reading JSON request bodies
   * - calling the authentication service
   * - returning standardized JSON responses
   *
   * Business rules belong in AuthService.
   * Database access belongs in AuthRepository.
   */
  class AuthController final
  {
  public:
    /**
     * @brief Registers a dashboard user.
     *
     * Expected JSON body:
     * {
     *   "name": "Gaspard",
     *   "email": "gaspard@example.com",
     *   "password": "secret"
     * }
     */
    static void register_user(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Logs in a dashboard user.
     *
     * Expected JSON body:
     * {
     *   "email": "gaspard@example.com",
     *   "password": "secret"
     * }
     */
    static void login(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Returns information about the current authenticated user.
     *
     * This endpoint will be fully implemented when dashboard token auth is
     * added to the middleware layer.
     */
    static void me(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    AuthController() = delete;
  };

} // namespace softadastra::cloud::modules::auth

#endif // SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_CONTROLLER_HPP
