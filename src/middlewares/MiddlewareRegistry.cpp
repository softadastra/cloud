/*
 * MiddlewareRegistry.cpp
 */

#include "middlewares/MiddlewareRegistry.hpp"

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#include <vix/log/Log.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/app_middleware.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/middleware/security/cors.hpp>

#include "middlewares/AgentAuthMiddleware.hpp"
#include "middlewares/RequestIdMiddleware.hpp"

namespace softadastra::cloud::middlewares
{
  namespace
  {
    [[nodiscard]] vix::App::Middleware cors_middleware(
        const softadastra::cloud::app::AppState &state)
    {
      vix::middleware::security::CorsOptions options;

      options.allowed_origins = {
          state.config.cors_allowed_origin,
      };

      options.allow_any_origin = false;
      options.allow_credentials = true;

      options.allow_methods = {
          "GET",
          "POST",
          "PUT",
          "PATCH",
          "DELETE",
          "OPTIONS",
      };

      options.allow_headers = {
          "Content-Type",
          "Authorization",
          "Accept",
          "Origin",
          "X-Requested-With",
          "X-Agent-Api-Key",
          "X-Request-Id",
      };

      options.expose_headers = {
          "X-Request-Id",
      };

      return vix::middleware::app::adapt_ctx(
          vix::middleware::security::cors(std::move(options)));
    }

    [[nodiscard]] vix::App::Middleware body_limit_middleware(
        const softadastra::cloud::app::AppState &state)
    {
      return vix::middleware::app::body_limit_write_dev(
          static_cast<std::size_t>(state.config.max_request_body_bytes));
    }

    [[nodiscard]] vix::App::Middleware rate_limit_middleware()
    {
      return vix::middleware::app::rate_limit_dev(
          120,
          std::chrono::minutes(1));
    }
  }

  void MiddlewareRegistry::install(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    vix::log::info("installing Softadastra Cloud middlewares");

    install_global_middlewares(app, state);
    install_agent_middlewares(app, state);

    vix::log::info("Softadastra Cloud middlewares installed");
  }

  void MiddlewareRegistry::install_global_middlewares(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.use(RequestIdMiddleware::create());

    app.use(
        vix::middleware::app::security_headers_dev(
            state.config.is_production()));

    app.use(cors_middleware(state));

    app.use(rate_limit_middleware());

    app.use(body_limit_middleware(state));
  }

  void MiddlewareRegistry::install_agent_middlewares(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    vix::middleware::app::install(
        app,
        "/api/agent/",
        AgentAuthMiddleware::create(state));

    vix::middleware::app::install(
        app,
        "/api/runs",
        AgentAuthMiddleware::create(state));
  }

} // namespace softadastra::cloud::middlewares
