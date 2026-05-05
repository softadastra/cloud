/**
 *
 *  @file TokenService.hpp
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

#ifndef SOFTADASTRA_CLOUD_SECURITY_TOKEN_SERVICE_HPP
#define SOFTADASTRA_CLOUD_SECURITY_TOKEN_SERVICE_HPP

#include <string>

#include "config/CloudConfig.hpp"

namespace softadastra::cloud::security
{
  /**
   * @brief Dashboard token helper.
   *
   * TokenService centralizes token creation for dashboard users.
   *
   * MVP note:
   * This creates a simple signed token format:
   *
   *   v1.<user_id>.<email>.<issued_at>.<signature>
   *
   * It is enough to keep the auth flow isolated while building the backend.
   * Before production, this can be replaced with a real JWT implementation
   * without changing AuthController or AuthService.
   */
  class TokenService final
  {
  public:
    /**
     * @brief Issues a dashboard user token.
     *
     * @param config Cloud configuration.
     * @param user_public_id Public user id.
     * @param email User email.
     * @return Signed token string.
     */
    [[nodiscard]] static std::string issue_user_token(
        const softadastra::cloud::config::CloudConfig &config,
        const std::string &user_public_id,
        const std::string &email);

    /**
     * @brief Verifies whether a token signature matches the configured secret.
     *
     * @param config Cloud configuration.
     * @param token Token string.
     * @return true when token is structurally valid and signature matches.
     */
    [[nodiscard]] static bool verify_user_token(
        const softadastra::cloud::config::CloudConfig &config,
        const std::string &token);

  private:
    TokenService() = delete;

    [[nodiscard]] static std::string sign(
        const softadastra::cloud::config::CloudConfig &config,
        const std::string &payload);

    [[nodiscard]] static bool constant_time_equals(
        const std::string &a,
        const std::string &b) noexcept;
  };

} // namespace softadastra::cloud::security

#endif // SOFTADASTRA_CLOUD_SECURITY_TOKEN_SERVICE_HPP
