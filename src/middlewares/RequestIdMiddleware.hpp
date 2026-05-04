/**
 *
 *  @file RequestIdMiddleware.hpp
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

#ifndef SOFTADASTRA_CLOUD_MIDDLEWARES_REQUEST_ID_MIDDLEWARE_HPP
#define SOFTADASTRA_CLOUD_MIDDLEWARES_REQUEST_ID_MIDDLEWARE_HPP

#include <vix.hpp>

namespace softadastra::cloud::middlewares
{
  /**
   * @brief Request id middleware for Softadastra Cloud.
   *
   * This middleware creates one request id per HTTP request, stores it in
   * RequestState through RequestContext, and exposes it as an HTTP response
   * header.
   *
   * Header:
   * - x-request-id
   */
  class RequestIdMiddleware final
  {
  public:
    /**
     * @brief Creates the Vix middleware instance.
     *
     * @return Vix app middleware.
     */
    [[nodiscard]] static vix::App::Middleware create();

  private:
    RequestIdMiddleware() = delete;
  };

} // namespace softadastra::cloud::middlewares

#endif // SOFTADASTRA_CLOUD_MIDDLEWARES_REQUEST_ID_MIDDLEWARE_HPP
