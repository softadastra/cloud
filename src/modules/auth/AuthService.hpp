/**
 *
 *  @file AuthService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_SERVICE_HPP

#include <string>

#include "app/AppState.hpp"
#include "modules/auth/AuthRepository.hpp"

namespace softadastra::cloud::modules::auth
{
  /**
   * @brief Result returned by authentication operations.
   */
  struct AuthResult
  {
    bool success{false};
    int status{400};

    std::string error{"auth_failed"};
    std::string message{};

    std::string user_public_id{};
    std::string email{};
    std::string name{};
    std::string token{};
  };

  /**
   * @brief Authentication business service.
   *
   * AuthService owns the authentication rules:
   * - validate register/login input
   * - check whether an email already exists
   * - create users
   * - verify login credentials
   * - issue dashboard tokens
   *
   * It does not read Vix Request and does not write Vix Response.
   */
  class AuthService final
  {
  public:
    /**
     * @brief Creates an auth service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit AuthService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Registers a new dashboard user.
     *
     * @param name User display name.
     * @param email User email.
     * @param password Raw password.
     * @return Auth result.
     */
    [[nodiscard]] AuthResult register_user(
        const std::string &name,
        const std::string &email,
        const std::string &password);

    /**
     * @brief Authenticates a dashboard user.
     *
     * @param email User email.
     * @param password Raw password.
     * @return Auth result.
     */
    [[nodiscard]] AuthResult login(
        const std::string &email,
        const std::string &password);

  private:
    const softadastra::cloud::app::AppState &state_;
    AuthRepository repository_;

    [[nodiscard]] static bool is_valid_email(
        const std::string &email) noexcept;

    [[nodiscard]] static bool is_valid_password(
        const std::string &password) noexcept;

    [[nodiscard]] static std::string hash_password(
        const std::string &password);

    [[nodiscard]] static bool verify_password(
        const std::string &password,
        const std::string &password_hash);

    [[nodiscard]] static AuthResult validation_error(
        std::string message);

    [[nodiscard]] static AuthResult unauthorized(
        std::string message);

    [[nodiscard]] static AuthResult conflict(
        std::string message);

    [[nodiscard]] static AuthResult internal_error(
        std::string message);
  };

} // namespace softadastra::cloud::modules::auth

#endif // SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_SERVICE_HPP
