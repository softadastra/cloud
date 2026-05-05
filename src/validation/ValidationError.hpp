/**
 *
 *  @file ValidationError.hpp
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

#ifndef SOFTADASTRA_CLOUD_VALIDATION_VALIDATION_ERROR_HPP
#define SOFTADASTRA_CLOUD_VALIDATION_VALIDATION_ERROR_HPP

#include <string>
#include <utility>

namespace softadastra::cloud::validation
{
  /**
   * @brief Small validation error object used by validators.
   *
   * Validators should not write HTTP responses directly. They return this
   * object so controllers or services can decide how to expose the error.
   */
  struct ValidationError
  {
    std::string field{};
    std::string code{};
    std::string message{};

    ValidationError() = default;

    ValidationError(
        std::string error_field,
        std::string error_code,
        std::string error_message)
        : field(std::move(error_field)),
          code(std::move(error_code)),
          message(std::move(error_message))
    {
    }

    [[nodiscard]] bool is_valid() const noexcept
    {
      return !field.empty() &&
             !code.empty() &&
             !message.empty();
    }

    [[nodiscard]] bool valid() const noexcept
    {
      return is_valid();
    }
  };

} // namespace softadastra::cloud::validation

#endif // SOFTADASTRA_CLOUD_VALIDATION_VALIDATION_ERROR_HPP
