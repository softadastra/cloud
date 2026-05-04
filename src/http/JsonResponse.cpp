/*
 * JsonResponse.cpp
 */

#include "http/JsonResponse.hpp"

#include <string>

namespace softadastra::cloud::http
{
  void JsonResponse::ok(
      vix::Response &res,
      std::string_view message)
  {
    res.json(success_body(message));
  }

  void JsonResponse::data(
      vix::Response &res,
      const J::token &data)
  {
    res.json(data_body("ok", data));
  }

  void JsonResponse::data(
      vix::Response &res,
      std::string_view message,
      const J::token &data)
  {
    res.json(data_body(message, data));
  }

  void JsonResponse::created(
      vix::Response &res,
      const J::token &data)
  {
    res.status(201).json(data_body("created", data));
  }

  void JsonResponse::error(
      vix::Response &res,
      int status,
      std::string_view code,
      std::string_view message)
  {
    res.status(status).json(
        error_body(
            status,
            code,
            message));
  }

  void JsonResponse::validation_error(
      vix::Response &res,
      std::string_view message)
  {
    error(
        res,
        400,
        "validation_failed",
        message);
  }

  void JsonResponse::unauthorized(
      vix::Response &res,
      std::string_view message)
  {
    error(
        res,
        401,
        "unauthorized",
        message);
  }

  void JsonResponse::forbidden(
      vix::Response &res,
      std::string_view message)
  {
    error(
        res,
        403,
        "forbidden",
        message);
  }

  void JsonResponse::not_found(
      vix::Response &res,
      std::string_view message)
  {
    error(
        res,
        404,
        "not_found",
        message);
  }

  void JsonResponse::internal_error(
      vix::Response &res,
      std::string_view message)
  {
    error(
        res,
        500,
        "internal_error",
        message);
  }

  J::kvs JsonResponse::success_body(
      std::string_view message)
  {
    return J::obj({
        "ok",
        true,
        "message",
        std::string(message),
    });
  }

  J::kvs JsonResponse::data_body(
      std::string_view message,
      const J::token &data)
  {
    return J::obj({
        "ok",
        true,
        "message",
        std::string(message),
        "data",
        data,
    });
  }

  J::kvs JsonResponse::error_body(
      int status,
      std::string_view code,
      std::string_view message)
  {
    return J::obj({
        "ok",
        false,
        "status",
        static_cast<long long>(status),
        "error",
        std::string(code),
        "message",
        std::string(message),
    });
  }

} // namespace softadastra::cloud::http
