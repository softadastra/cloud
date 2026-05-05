/**
 *
 *  @file Time.hpp
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

#ifndef SOFTADASTRA_CLOUD_UTILS_TIME_HPP
#define SOFTADASTRA_CLOUD_UTILS_TIME_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::utils
{
  /**
   * @brief Time helper for Softadastra Cloud.
   *
   * Time centralizes timestamp creation and formatting for the backend.
   *
   * Internally, database timestamps are stored as milliseconds since Unix
   * epoch in UTC.
   */
  class Time final
  {
  public:
    /**
     * @brief Returns current Unix timestamp in milliseconds.
     *
     * @return Current UTC timestamp in milliseconds.
     */
    [[nodiscard]] static std::int64_t now_ms();

    /**
     * @brief Returns current Unix timestamp in seconds.
     *
     * @return Current UTC timestamp in seconds.
     */
    [[nodiscard]] static std::int64_t now_seconds();

    /**
     * @brief Returns current UTC datetime as a string.
     *
     * @return ISO-like UTC datetime string.
     */
    [[nodiscard]] static std::string now_utc_string();

    /**
     * @brief Converts milliseconds to seconds.
     *
     * @param millis Timestamp in milliseconds.
     * @return Timestamp in seconds.
     */
    [[nodiscard]] static std::int64_t ms_to_seconds(
        std::int64_t millis) noexcept;

    /**
     * @brief Converts seconds to milliseconds.
     *
     * @param seconds Timestamp in seconds.
     * @return Timestamp in milliseconds.
     */
    [[nodiscard]] static std::int64_t seconds_to_ms(
        std::int64_t seconds) noexcept;

  private:
    Time() = delete;
  };

} // namespace softadastra::cloud::utils

#endif // SOFTADASTRA_CLOUD_UTILS_TIME_HPP
