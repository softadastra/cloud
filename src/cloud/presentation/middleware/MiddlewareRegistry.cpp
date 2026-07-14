/**
 * @file MiddlewareRegistry.cpp
 * @brief Middleware registration for the cloud backend.
 */

#include <cloud/presentation/middleware/MiddlewareRegistry.hpp>

#include <auth/middleware/AuthMiddleware.hpp>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/log.hpp>
#include <vix/middleware.hpp>
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/security/cors.hpp>

namespace cloud::presentation::middleware
{
  namespace
  {
    std::string trim(std::string value)
    {
      const auto first = value.find_first_not_of(" \t\r\n");

      if (first == std::string::npos)
      {
        return {};
      }

      const auto last = value.find_last_not_of(" \t\r\n");
      return value.substr(first, last - first + 1);
    }

    std::vector<std::string> configured_cors_origins()
    {
      const char *raw = std::getenv("CORS_ALLOWED_ORIGINS");

      if (!raw || !*raw)
      {
        return {
          "http://localhost:5173",
          "http://127.0.0.1:5173"
        };
      }

      std::vector<std::string> origins;
      std::stringstream stream(raw);
      std::string origin;

      while (std::getline(stream, origin, ','))
      {
        origin = trim(std::move(origin));

        if (!origin.empty())
        {
          origins.push_back(std::move(origin));
        }
      }

      return origins;
    }

    bool is_allowed_origin(
      const std::vector<std::string> &allowed_origins,
      const std::string &origin)
    {
      return std::find(
               allowed_origins.begin(),
               allowed_origins.end(),
               origin
             ) != allowed_origins.end();
    }
  }

  void MiddlewareRegistry::register_all(vix::App &app)
  {
    // Recommended production order:
    // CORS -> rate limit -> request logging -> security headers -> body limits -> auth -> routes.

    const auto allowed_origins = configured_cors_origins();

    vix::middleware::security::CorsOptions cors_options{};
    cors_options.allowed_origins = allowed_origins;
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

    app.use(
      vix::middleware::app::adapt_ctx(
        vix::middleware::security::cors(std::move(cors_options))
      )
    );

    // Explicit fallback for browser preflight requests.
    app.use([allowed_origins](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      auto origin = req.header("Origin");

      if (origin.empty())
      {
        origin = req.header("origin");
      }

      if (is_allowed_origin(allowed_origins, origin))
      {
        res.header("Access-Control-Allow-Origin", origin);
        res.header("Access-Control-Allow-Credentials", "true");
        res.header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.header(
          "Access-Control-Allow-Headers",
          "Content-Type, Authorization, X-Session-Id, X-Access-Token, "
          "X-Requested-With, Accept, Origin"
        );
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

    app.use(vix::middleware::app::security_headers_dev(false));

    app.use([](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      (void)res;

      vix::log::info("{} {}", req.method(), req.path());
      next();
    });

    app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      (void)req;

      res.header("X-API", "true");
      next();
    });

    app.use("/api", [](vix::Request &req, vix::Response &res, vix::App::Next next)
    {
      if (!cloud::auth::middleware::require_api_auth_and_workspace_permission(req, res))
      {
        return;
      }

      next();
    });
  }
} // namespace cloud::presentation::middleware
