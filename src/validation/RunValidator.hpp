/**
 *
 *  @file RunValidator.hpp
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

#ifndef SOFTADASTRA_CLOUD_VALIDATION_RUN_VALIDATOR_HPP
#define SOFTADASTRA_CLOUD_VALIDATION_RUN_VALIDATOR_HPP

#include <optional>
#include <string>

#include "validation/ValidationError.hpp"

namespace softadastra::cloud::validation
{
  /**
   * @brief Validator for reliability run input.
   *
   * RunValidator centralizes reusable validation rules for run payloads.
   *
   * Controllers and services can use it without depending on HTTP or database
   * code.
   */
  class RunValidator final
  {
  public:
    /**
     * @brief Validates a target URL.
     *
     * @param target_url Target URL.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_target_url(
        const std::string &target_url);

    /**
     * @brief Validates an HTTP method.
     *
     * @param method HTTP method.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_method(
        const std::string &method);

    /**
     * @brief Validates a scenario name.
     *
     * @param scenario Reliability scenario.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_scenario(
        const std::string &scenario);

    /**
     * @brief Validates a run status.
     *
     * @param status Run status.
     * @return Validation error when invalid.
     */
    [[nodiscard]] static std::optional<ValidationError> validate_status(
        const std::string &status);

    /**
     * @brief Returns true when the status is a terminal status.
     *
     * @param status Run status.
     * @return true when terminal.
     */
    [[nodiscard]] static bool is_final_status(
        const std::string &status) noexcept;

    /**
     * @brief Normalizes an HTTP method to uppercase.
     *
     * @param method HTTP method.
     * @return Uppercase method.
     */
    [[nodiscard]] static std::string normalize_method(
        const std::string &method);

  private:
    RunValidator() = delete;

    [[nodiscard]] static bool starts_with(
        const std::string &value,
        const std::string &prefix) noexcept;
  };

} // namespace softadastra::cloud::validation

#endif // SOFTADASTRA_CLOUD_VALIDATION_RUN_VALIDATOR_HPP
