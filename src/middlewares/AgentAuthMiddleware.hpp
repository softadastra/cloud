/**
 *
 *  @file AgentAuthMiddleware.hpp
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

#ifndef SOFTADASTRA_CLOUD_MIDDLEWARES_AGENT_AUTH_MIDDLEWARE_HPP
#define SOFTADASTRA_CLOUD_MIDDLEWARES_AGENT_AUTH_MIDDLEWARE_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::middlewares
{
  /**
   * @brief Agent API key authentication middleware.
   *
   * This middleware protects endpoints used by Softadastra agents.
   *
   * For the first version, it validates a shared agent API key from config.
   * Later, this middleware will verify hashed per-agent keys from the database.
   *
   * Expected header:
   * - x-agent-api-key
   */
  class AgentAuthMiddleware final
  {
  public:
    /**
     * @brief Creates the agent authentication middleware.
     *
     * @param state Shared application state.
     * @return Vix app middleware.
     */
    [[nodiscard]] static vix::App::Middleware create(
        const softadastra::cloud::app::AppState &state);

  private:
    AgentAuthMiddleware() = delete;
  };

} // namespace softadastra::cloud::middlewares

#endif // SOFTADASTRA_CLOUD_MIDDLEWARES_AGENT_AUTH_MIDDLEWARE_HPP
