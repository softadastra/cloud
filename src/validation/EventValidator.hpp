/**
 *
 *  @file EventValidator.hpp
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

#ifndef SOFTADASTRA_CLOUD_VALIDATION_EVENT_VALIDATOR_HPP
#define SOFTADASTRA_CLOUD_VALIDATION_EVENT_VALIDATOR_HPP

#include <optional>
#include <string>

#include "validation/ValidationError.hpp"

namespace softadastra::cloud::validation
{
  /**
   * @brief Validator for run event input.
   *
   * EventValidator centralizes reusable validation rules for event payloads.
   *
   * It does not depend on HTTP, Vix Request/Response, services, or database
   * repositories.
   */
  class EventValidator final
  {
  public:
    /**
     * @brief Validates an event type.
     *
     * @param event_type Event type.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_event_type(
        const std::string &event_type);

    /**
     * @brief Validates an event severity.
     *
     * @param severity Event severity.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_severity(
        const std::string &severity);

    /**
     * @brief Validates an event message.
     *
     * @param message Event message.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_message(
        const std::string &message);

    /**
     * @brief Validates an event payload string.
     *
     * @param payload Raw JSON payload string.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_payload(
        const std::string &payload);

    /**
     * @brief Normalizes severity values.
     *
     * Converts:
     * - WARN -> warning
     * - warn -> warning
     *
     * @param severity Event severity.
     * @return Normalized severity.
     */
    [[nodiscard]] static std::string normalize_severity(
        const std::string &severity);

  private:
    EventValidator() = delete;
  };

} // namespace softadastra::cloud::validation

#endif // SOFTADASTRA_CLOUD_VALIDATION_EVENT_VALIDATOR_HPP
