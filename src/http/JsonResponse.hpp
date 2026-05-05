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
#include <vix/json/convert.hpp>
#include <vix/json/json.hpp>

namespace softadastra::cloud::http
{
  namespace J = vix::json;

  /**
   * @brief Standard JSON response helpers for Softadastra Cloud.
   *
   * JsonResponse centralizes API response formatting so all controllers
   * return a predictable JSON structure.
   *
   * Success response:
   * @code
   * {
   *   "ok": true,
   *   "message": "ok",
   *   "data": {}
   * }
   * @endcode
   *
   * Error response:
   * @code
   * {
   *   "ok": false,
   *   "status": 400,
   *   "error": "validation_failed",
   *   "message": "Invalid request"
   * }
   * @endcode
   *
   * The class accepts both modern Vix JSON values and lightweight Simple JSON
   * values:
   * - J::Json
   * - J::OrderedJson
   * - J::kvs
   * - J::token
   *
   * Internally, all payloads are normalized to J::Json before being sent.
   */
  class JsonResponse final
  {
  public:
    /**
     * @brief Sends a simple success response.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable success message.
     */
    static void ok(
        vix::Response &res,
        std::string_view message = "ok");

    /**
     * @brief Sends a success response with a JSON payload.
     *
     * @param res Vix response wrapper.
     * @param data JSON data payload.
     */
    static void data(
        vix::Response &res,
        const J::Json &data);

    /**
     * @brief Sends a success response with an ordered JSON payload.
     *
     * @param res Vix response wrapper.
     * @param data Ordered JSON data payload.
     */
    static void data(
        vix::Response &res,
        const J::OrderedJson &data);

    /**
     * @brief Sends a success response with a Simple JSON object payload.
     *
     * @param res Vix response wrapper.
     * @param data Simple JSON key-value payload.
     */
    static void data(
        vix::Response &res,
        const J::kvs &data);

    /**
     * @brief Sends a success response with a Simple JSON token payload.
     *
     * @param res Vix response wrapper.
     * @param data Simple JSON token payload.
     */
    static void data(
        vix::Response &res,
        const J::token &data);

    /**
     * @brief Sends a success response with message and JSON payload.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable success message.
     * @param data JSON data payload.
     */
    static void data(
        vix::Response &res,
        std::string_view message,
        const J::Json &data);

    /**
     * @brief Sends a success response with message and ordered JSON payload.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable success message.
     * @param data Ordered JSON data payload.
     */
    static void data(
        vix::Response &res,
        std::string_view message,
        const J::OrderedJson &data);

    /**
     * @brief Sends a success response with message and Simple JSON object payload.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable success message.
     * @param data Simple JSON key-value payload.
     */
    static void data(
        vix::Response &res,
        std::string_view message,
        const J::kvs &data);

    /**
     * @brief Sends a success response with message and Simple JSON token payload.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable success message.
     * @param data Simple JSON token payload.
     */
    static void data(
        vix::Response &res,
        std::string_view message,
        const J::token &data);

    /**
     * @brief Sends a 201 Created response with a JSON payload.
     *
     * @param res Vix response wrapper.
     * @param data JSON data payload.
     */
    static void created(
        vix::Response &res,
        const J::Json &data);

    /**
     * @brief Sends a 201 Created response with an ordered JSON payload.
     *
     * @param res Vix response wrapper.
     * @param data Ordered JSON data payload.
     */
    static void created(
        vix::Response &res,
        const J::OrderedJson &data);

    /**
     * @brief Sends a 201 Created response with a Simple JSON object payload.
     *
     * @param res Vix response wrapper.
     * @param data Simple JSON key-value payload.
     */
    static void created(
        vix::Response &res,
        const J::kvs &data);

    /**
     * @brief Sends a 201 Created response with a Simple JSON token payload.
     *
     * @param res Vix response wrapper.
     * @param data Simple JSON token payload.
     */
    static void created(
        vix::Response &res,
        const J::token &data);

    /**
     * @brief Sends an error response.
     *
     * @param res Vix response wrapper.
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
     * @brief Sends a 400 validation error response.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable validation message.
     */
    static void validation_error(
        vix::Response &res,
        std::string_view message);

    /**
     * @brief Sends a 401 unauthorized response.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable error message.
     */
    static void unauthorized(
        vix::Response &res,
        std::string_view message = "Unauthorized");

    /**
     * @brief Sends a 403 forbidden response.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable error message.
     */
    static void forbidden(
        vix::Response &res,
        std::string_view message = "Forbidden");

    /**
     * @brief Sends a 404 not found response.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable error message.
     */
    static void not_found(
        vix::Response &res,
        std::string_view message = "Resource not found");

    /**
     * @brief Sends a 500 internal server error response.
     *
     * @param res Vix response wrapper.
     * @param message Human-readable error message.
     */
    static void internal_error(
        vix::Response &res,
        std::string_view message = "Internal server error");

  private:
    JsonResponse() = delete;

    /**
     * @brief Normalizes a JSON payload.
     *
     * @param data JSON payload.
     * @return Normalized JSON payload.
     */
    [[nodiscard]] static J::Json normalize_data(
        const J::Json &data);

    /**
     * @brief Normalizes an ordered JSON payload.
     *
     * @param data Ordered JSON payload.
     * @return Normalized JSON payload.
     */
    [[nodiscard]] static J::Json normalize_data(
        const J::OrderedJson &data);

    /**
     * @brief Normalizes a Simple JSON object payload.
     *
     * @param data Simple JSON key-value payload.
     * @return Normalized JSON payload.
     */
    [[nodiscard]] static J::Json normalize_data(
        const J::kvs &data);

    /**
     * @brief Normalizes a Simple JSON token payload.
     *
     * @param data Simple JSON token payload.
     * @return Normalized JSON payload.
     */
    [[nodiscard]] static J::Json normalize_data(
        const J::token &data);

    /**
     * @brief Builds a standard success response body.
     *
     * @param message Human-readable success message.
     * @return Ordered JSON response body.
     */
    [[nodiscard]] static J::OrderedJson success_body(
        std::string_view message);

    /**
     * @brief Builds a standard data response body.
     *
     * @param message Human-readable success message.
     * @param data Normalized JSON data payload.
     * @return Ordered JSON response body.
     */
    [[nodiscard]] static J::OrderedJson data_body(
        std::string_view message,
        const J::Json &data);

    /**
     * @brief Builds a standard error response body.
     *
     * @param status HTTP status code.
     * @param code Stable machine-readable error code.
     * @param message Human-readable error message.
     * @return Ordered JSON response body.
     */
    [[nodiscard]] static J::OrderedJson error_body(
        int status,
        std::string_view code,
        std::string_view message);
  };

} // namespace softadastra::cloud::http

#endif // SOFTADASTRA_CLOUD_HTTP_JSON_RESPONSE_HPP
