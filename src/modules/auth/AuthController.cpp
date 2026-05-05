/*
 * AuthController.cpp
 */

#include "modules/auth/AuthController.hpp"

#include <exception>
#include <string>

#include <vix/log/Log.hpp>
#include <vix/json/json.hpp>
#include <vix/json/convert.hpp>

#include "http/JsonResponse.hpp"
#include "modules/auth/AuthService.hpp"

namespace softadastra::cloud::modules::auth
{
  namespace J = vix::json;

  namespace
  {
    [[nodiscard]] bool has_string_field(
        const vix::json::Json &body,
        std::string_view field)
    {
      return body.is_object() &&
             body.contains(std::string(field)) &&
             body.at(std::string(field)).is_string();
    }

    [[nodiscard]] std::string string_field_or_empty(
        const vix::json::Json &body,
        std::string_view field)
    {
      return vix::json::get_or<std::string>(
          body,
          field,
          "");
    }
  }

  void AuthController::register_user(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const auto &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      const std::string name =
          string_field_or_empty(body, "name");

      const std::string email =
          string_field_or_empty(body, "email");

      const std::string password =
          string_field_or_empty(body, "password");

      AuthService service{state};

      const auto result =
          service.register_user(
              name,
              email,
              password);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::created(
          res,
          J::obj({
              "user_id",
              result.user_public_id,
              "email",
              result.email,
              "name",
              result.name,
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "register user failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AuthController::login(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const auto &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      const std::string email =
          string_field_or_empty(body, "email");

      const std::string password =
          string_field_or_empty(body, "password");

      AuthService service{state};

      const auto result =
          service.login(
              email,
              password);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          "authenticated",
          J::obj({
              "token",
              result.token,
              "user",
              J::obj({
                  "user_id",
                  result.user_public_id,
                  "email",
                  result.email,
                  "name",
                  result.name,
              }),
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "login failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void AuthController::me(
      vix::Request &,
      vix::Response &res,
      const softadastra::cloud::app::AppState &)
  {
    softadastra::cloud::http::JsonResponse::error(
        res,
        501,
        "not_implemented",
        "Dashboard token authentication is not implemented yet");
  }

} // namespace softadastra::cloud::modules::auth
