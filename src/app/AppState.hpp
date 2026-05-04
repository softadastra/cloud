/**
 *
 *  @file AppState.hpp
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

#ifndef SOFTADASTRA_CLOUD_APP_APP_STATE_HPP
#define SOFTADASTRA_CLOUD_APP_APP_STATE_HPP

#include <memory>

#include <vix/db/db.hpp>

#include "config/CloudConfig.hpp"

namespace softadastra::cloud::app
{
  /**
   * @brief Shared application state for Softadastra Cloud.
   *
   * AppState owns the shared dependencies used by routes, controllers,
   * services, repositories, and middlewares.
   *
   * It keeps the Vix App lightweight and avoids global variables.
   */
  struct AppState
  {
    /**
     * @brief Application-level cloud configuration.
     */
    softadastra::cloud::config::CloudConfig config;

    /**
     * @brief Shared database connection manager.
     */
    std::shared_ptr<vix::db::Database> database{};

    /**
     * @brief Returns true if all required dependencies are available.
     */
    [[nodiscard]] bool is_valid() const noexcept
    {
      return config.is_valid() && database != nullptr;
    }

    /**
     * @brief Backward-compatible valid alias.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return is_valid();
    }
  };

} // namespace softadastra::cloud::app

#endif // SOFTADASTRA_CLOUD_APP_APP_STATE_HPP
