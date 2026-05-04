/**
 *
 *  @file ApiKeyHasher.hpp
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

#ifndef SOFTADASTRA_CLOUD_SECURITY_API_KEY_HASHER_HPP
#define SOFTADASTRA_CLOUD_SECURITY_API_KEY_HASHER_HPP

#include <string>

#include "config/CloudConfig.hpp"

namespace softadastra::cloud::security
{
  /**
   * @brief Helper for generating and hashing agent API keys.
   *
   * ApiKeyHasher centralizes API key handling so services do not implement
   * hashing details directly.
   *
   * MVP note:
   * This uses a deterministic hash wrapper suitable for local development.
   * Before production, replace the implementation with a stronger keyed hash
   * such as HMAC-SHA256.
   */
  class ApiKeyHasher final
  {
  public:
    /**
     * @brief Generates a new plain agent API key.
     *
     * The plain key must only be shown once to the caller.
     *
     * @return Plain API key.
     */
    [[nodiscard]] static std::string generate_agent_key();

    /**
     * @brief Hashes a plain API key using cloud config secret.
     *
     * @param config Cloud configuration.
     * @param api_key Plain API key.
     * @return Stable API key hash.
     */
    [[nodiscard]] static std::string hash(
        const softadastra::cloud::config::CloudConfig &config,
        const std::string &api_key);

    /**
     * @brief Verifies a plain API key against an existing hash.
     *
     * @param config Cloud configuration.
     * @param api_key Plain API key.
     * @param expected_hash Stored API key hash.
     * @return true when the key matches.
     */
    [[nodiscard]] static bool verify(
        const softadastra::cloud::config::CloudConfig &config,
        const std::string &api_key,
        const std::string &expected_hash);

  private:
    ApiKeyHasher() = delete;

    [[nodiscard]] static bool constant_time_equals(
        const std::string &a,
        const std::string &b) noexcept;
  };

} // namespace softadastra::cloud::security

#endif // SOFTADASTRA_CLOUD_SECURITY_API_KEY_HASHER_HPP
