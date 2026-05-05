/**
 *
 *  @file RequestContext.hpp
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

#ifndef SOFTADASTRA_CLOUD_HTTP_REQUEST_CONTEXT_HPP
#define SOFTADASTRA_CLOUD_HTTP_REQUEST_CONTEXT_HPP

#include <string>

namespace softadastra::cloud::http
{
  /**
   * @brief Request-scoped metadata used across middlewares and route handlers.
   *
   * RequestContext stores information generated during request processing,
   * such as the request id and authenticated agent identity.
   *
   * It is intentionally small and plain so it can be stored inside Vix
   * RequestState.
   */
  struct RequestContext
  {
    /**
     * @brief Unique id assigned to the current request.
     */
    std::string request_id{};

    /**
     * @brief Public id of the authenticated agent.
     *
     * Empty when the request is not authenticated as an agent.
     */
    std::string agent_public_id{};

    /**
     * @brief Public id of the project attached to the authenticated agent.
     *
     * Empty when no project context is attached.
     */
    std::string project_public_id{};

    /**
     * @brief Authentication source used for this request.
     *
     * Example values:
     * - none
     * - agent_api_key
     * - dashboard_token
     */
    std::string auth_source{"none"};

    /**
     * @brief Returns true if a request id is available.
     */
    [[nodiscard]] bool has_request_id() const noexcept;

    /**
     * @brief Returns true if this request has an authenticated agent.
     */
    [[nodiscard]] bool has_agent() const noexcept;

    /**
     * @brief Returns true if this request has a project context.
     */
    [[nodiscard]] bool has_project() const noexcept;

    /**
     * @brief Marks this request as authenticated by an agent API key.
     *
     * @param agent_id Public agent id.
     * @param project_id Public project id.
     */
    void set_agent_auth(
        std::string agent_id,
        std::string project_id);

    /**
     * @brief Clears authentication-related fields.
     *
     * The request id is intentionally preserved because it is useful for
     * tracing logs even when authentication fails.
     */
    void clear_auth() noexcept;
  };

} // namespace softadastra::cloud::http

#endif // SOFTADASTRA_CLOUD_HTTP_REQUEST_CONTEXT_HPP
