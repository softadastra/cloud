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
    res.status(200).json(success_body(message));
  }

  void JsonResponse::data(
      vix::Response &res,
      const J::Json &data)
  {
    JsonResponse::data(res, "ok", data);
  }

  void JsonResponse::data(
      vix::Response &res,
      const J::OrderedJson &data)
  {
    JsonResponse::data(res, "ok", data);
  }

  void JsonResponse::data(
      vix::Response &res,
      const J::kvs &data)
  {
    JsonResponse::data(res, "ok", data);
  }

  void JsonResponse::data(
      vix::Response &res,
      const J::token &data)
  {
    JsonResponse::data(res, "ok", data);
  }

  void JsonResponse::data(
      vix::Response &res,
      std::string_view message,
      const J::Json &data)
  {
    res.status(200).json(data_body(message, normalize_data(data)));
  }

  void JsonResponse::data(
      vix::Response &res,
      std::string_view message,
      const J::OrderedJson &data)
  {
    res.status(200).json(data_body(message, normalize_data(data)));
  }

  void JsonResponse::data(
      vix::Response &res,
      std::string_view message,
      const J::kvs &data)
  {
    res.status(200).json(data_body(message, normalize_data(data)));
  }

  void JsonResponse::data(
      vix::Response &res,
      std::string_view message,
      const J::token &data)
  {
    res.status(200).json(data_body(message, normalize_data(data)));
  }

  void JsonResponse::created(
      vix::Response &res,
      const J::Json &data)
  {
    res.status(201).json(data_body("created", normalize_data(data)));
  }

  void JsonResponse::created(
      vix::Response &res,
      const J::OrderedJson &data)
  {
    res.status(201).json(data_body("created", normalize_data(data)));
  }

  void JsonResponse::created(
      vix::Response &res,
      const J::kvs &data)
  {
    res.status(201).json(data_body("created", normalize_data(data)));
  }

  void JsonResponse::created(
      vix::Response &res,
      const J::token &data)
  {
    res.status(201).json(data_body("created", normalize_data(data)));
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

  J::Json JsonResponse::normalize_data(
      const J::Json &data)
  {
    return data;
  }

  J::Json JsonResponse::normalize_data(
      const J::OrderedJson &data)
  {
    return J::Json::parse(data.dump());
  }

  J::Json JsonResponse::normalize_data(
      const J::kvs &data)
  {
    return J::to_json(data);
  }

  J::Json JsonResponse::normalize_data(
      const J::token &data)
  {
    return J::to_json(data);
  }

  J::OrderedJson JsonResponse::success_body(
      std::string_view message)
  {
    return J::o(
        "ok",
        true,
        "message",
        std::string(message));
  }

  J::OrderedJson JsonResponse::data_body(
      std::string_view message,
      const J::Json &data)
  {
    return J::o(
        "ok",
        true,
        "message",
        std::string(message),
        "data",
        data);
  }

  J::OrderedJson JsonResponse::error_body(
      int status,
      std::string_view code,
      std::string_view message)
  {
    return J::o(
        "ok",
        false,
        "status",
        status,
        "error",
        std::string(code),
        "message",
        std::string(message));
  }

} // namespace softadastra::cloud::http
