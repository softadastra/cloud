/**
 * @file MiddlewareRegistry.cpp
 * @brief Middleware registration for the cloud backend.
 */

#include <cloud/presentation/middleware/MiddlewareRegistry.hpp>

#include <auth/middleware/AuthMiddleware.hpp>

#include <vix.hpp>
#include <vix/log.hpp>
#include <vix/middleware.hpp>

namespace cloud::presentation::middleware
{
  void MiddlewareRegistry::register_all(vix::App &app)
  {
    // Recommended production order:
    // CORS -> rate limit -> request logging -> security headers -> body limits -> auth -> routes.

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
