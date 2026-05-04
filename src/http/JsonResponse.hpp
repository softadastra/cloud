/**
 *
 *  @file JsonResponse.hpp
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

#ifndef SOFTADASTRA_CLOUD_HTTP_JSON_RESPONSE_HPP
#define SOFTADASTRA_CLOUD_HTTP_JSON_RESPONSE_HPP

#include <string>
#include <string_view>

#include <vix.hpp>
#include <vix/json/Simple.hpp>

namespace softadastra::cloud::http
{
  namespace J = vix::json;

  /**
   * @brief Standard JSON response helpers for Softadastra Cloud.
   *
   * JsonResponse keeps every API response predictable:
   *
   * Success:
   * {
   *   "ok": true,
   *   "data": ...
   * }
   *
   * Error:
   * {
   *   "ok": false,
   *   "error": "...",
   *   "message": "..."
   * }
   */
  class JsonResponse final
  {
  public:
    /**
     * @brief Sends a simple success response.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable message.
     */
    static void ok(
        vix::Response &res,
        std::string_view message = "ok");

    /**
     * @brief Sends a success response with data.
     *
     * @param res Vix HTTP response.
     * @param data JSON data payload.
     */
    static void data(
        vix::Response &res,
        const J::token &data);

    /**
     * @brief Sends a success response with message and data.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable message.
     * @param data JSON data payload.
     */
    static void data(
        vix::Response &res,
        std::string_view message,
        const J::token &data);

    /**
     * @brief Sends a created response with data.
     *
     * @param res Vix HTTP response.
     * @param data JSON data payload.
     */
    static void created(
        vix::Response &res,
        const J::token &data);

    /**
     * @brief Sends an error response.
     *
     * @param res Vix HTTP response.
     * @param status HTTP status code.
     * @param code Stable machine-readable error code.
     * @param message Human-readable error message.
     */
    static void error(
        vix::Response &res,
        int status,
        std::string_view code,
        std::string_view message);

    /**
     * @brief Sends a validation error response.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable validation message.
     */
    static void validation_error(
        vix::Response &res,
        std::string_view message);

    /**
     * @brief Sends an unauthorized response.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable error message.
     */
    static void unauthorized(
        vix::Response &res,
        std::string_view message = "Unauthorized");

    /**
     * @brief Sends a forbidden response.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable error message.
     */
    static void forbidden(
        vix::Response &res,
        std::string_view message = "Forbidden");

    /**
     * @brief Sends a not found response.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable error message.
     */
    static void not_found(
        vix::Response &res,
        std::string_view message = "Resource not found");

    /**
     * @brief Sends an internal server error response.
     *
     * @param res Vix HTTP response.
     * @param message Human-readable error message.
     */
    static void internal_error(
        vix::Response &res,
        std::string_view message = "Internal server error");

  private:
    JsonResponse() = delete;

    [[nodiscard]] static J::kvs success_body(
        std::string_view message);

    [[nodiscard]] static J::kvs data_body(
        std::string_view message,
        const J::token &data);

    [[nodiscard]] static J::kvs error_body(
        int status,
        std::string_view code,
        std::string_view message);
  };

} // namespace softadastra::cloud::http

#endif // SOFTADASTRA_CLOUD_HTTP_JSON_RESPONSE_HPP
