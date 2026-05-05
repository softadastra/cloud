/**
 *
 *  @file AppFactory.hpp
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

#ifndef SOFTADASTRA_CLOUD_APP_APP_FACTORY_HPP
#define SOFTADASTRA_CLOUD_APP_APP_FACTORY_HPP

#include <memory>

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::app
{
  /**
   * @brief Factory responsible for building the Softadastra Cloud Vix app.
   *
   * AppFactory creates the HTTP application, installs middlewares, and
   * registers all routes.
   *
   * It keeps main.cpp small and makes application bootstrap easier to test.
   */
  class AppFactory final
  {
  public:
    /**
     * @brief Creates a configured Vix application.
     *
     * @param state Shared application state.
     * @return Configured Vix App instance.
     */
    [[nodiscard]] static std::unique_ptr<vix::App> create(const AppState &state);

  private:
    AppFactory() = delete;
  };

} // namespace softadastra::cloud::app

#endif // SOFTADASTRA_CLOUD_APP_APP_FACTORY_HPP
