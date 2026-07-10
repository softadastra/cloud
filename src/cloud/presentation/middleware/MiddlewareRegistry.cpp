/**
 * @file MiddlewareRegistry.cpp
 * @brief Middleware registration for the cloud backend.
 */

#include <cloud/presentation/middleware/MiddlewareRegistry.hpp>

#include <auth/middleware/AuthMiddleware.hpp>

#include <string>

#include <vix.hpp>
#include <vix/log.hpp>
#include <vix/middleware.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/security/cors.hpp>

namespace cloud::presentation::middleware
{
  void MiddlewareRegistry::register_all(vix::App &app)
  {
    // Recommended production order:
    // CORS -> rate limit -> request logging -> security headers -> body limits -> auth -> routes.

    // CORS for local dashboard development.
    vix::middleware::security::CorsOptions cors_options{};
    cors_options.allowed_origins = {
      "http://localhost:5173",
      "http://127.0.0.1:5173",
      "http://192.168.1.6:5173"
    };
    cors_options.allow_credentials = true;
    cors_options.allow_methods = {"GET", "POST", "OPTIONS"};
    cors_options.allow_headers = {
      "Content-Type",
      "Authorization",
      "X-Session-Id",
      "X-Access-Token",
      "X-Requested-With",
      "Accept",
      "Origin"
    };
    cors_options.expose_headers = {"X-Request-Id"};

    app.use(vix::middleware::app::adapt_ctx(vix::middleware::security::cors(std::move(cors_options))));

    // Explicit CORS fallback for browser preflight requests. The framework CORS
    // middleware handles most cases, but uploads with Authorization + image
    // Content-Type must always receive these headers or fetch reports a network error.
    app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      auto origin = req.header("Origin");

      if (origin.empty())
      {
        origin = req.header("origin");
      }

      if (origin == "http://localhost:5173" ||
          origin == "http://127.0.0.1:5173" ||
          origin == "http://192.168.1.6:5173")
      {
        res.header("Access-Control-Allow-Origin", origin);
        res.header("Access-Control-Allow-Credentials", "true");
        res.header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Session-Id, X-Access-Token, X-Requested-With, Accept, Origin");
        res.header("Access-Control-Expose-Headers", "X-Request-Id");
        res.header("Vary", "Origin");
      }

      if (req.method() == "OPTIONS")
      {
        res.status(204);
        return;
      }

      next();
    });

    // Security headers.
    app.use(vix::middleware::app::security_headers_dev(false));

    // Request logging.
    app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      (void)res;

      vix::log::info("{} {}", req.method(), req.path());
      next();
    });

    // Basic API marker header.
    app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      (void)req;

      res.header("X-API", "true");
      next();
    });

    // MVP auth and workspace permissions for protected API routes.
    app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      if (!cloud::auth::middleware::require_api_auth_and_workspace_permission(req, res))
      {
        return;
      }

      next();
    });

    // Optional examples for real applications:
    //
    // app.use(vix::middleware::app::cors_dev({
    //   "http://localhost:5173",
    //   "http://127.0.0.1:5173"
    // }));
    //
    // app.use(vix::middleware::app::rate_limit({
    //   .max_requests = 60,
    //   .window_seconds = 60
    // }));
  }
} // namespace cloud::presentation::middleware
