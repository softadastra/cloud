/*
 * HealthController.cpp
 */

#include "modules/health/HealthController.hpp"

#include <exception>
#include <string>

#include <vix/log/Log.hpp>

#include "http/JsonResponse.hpp"
#include "http/RequestContext.hpp"

namespace softadastra::cloud::modules::health
{
  namespace J = vix::json;

  namespace
  {
    [[nodiscard]] std::string request_id_from(
        const vix::Request &req)
    {
      const auto *context =
          req.try_state<softadastra::cloud::http::RequestContext>();

      if (context == nullptr)
      {
        return "";
      }

      return context->request_id;
    }
  }

  void HealthController::health(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    softadastra::cloud::http::JsonResponse::data(
        res,
        "healthy",
        J::obj({
            "service",
            state.config.service_name,
            "status",
            "healthy",
            "environment",
            state.config.environment,
            "request_id",
            request_id_from(req),
        }));
  }

  void HealthController::readiness(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      auto rows =
          state.database->query("SELECT 1");

      bool database_ok = false;

      if (rows->next())
      {
        database_ok = rows->row().getInt64(0) == 1;
      }

      if (!database_ok)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            503,
            "database_unavailable",
            "Database readiness check failed");

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          "ready",
          J::obj({
              "service",
              state.config.service_name,
              "status",
              "ready",
              "environment",
              state.config.environment,
              "database",
              true,
              "request_id",
              request_id_from(req),
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "readiness check failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::error(
          res,
          503,
          "database_unavailable",
          "Database readiness check failed");
    }
  }

} // namespace softadastra::cloud::modules::health
