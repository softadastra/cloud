/**
 *
 *  @file CloudConfig.hpp
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

#ifndef SOFTADASTRA_CLOUD_CONFIG_CLOUD_CONFIG_HPP
#define SOFTADASTRA_CLOUD_CONFIG_CLOUD_CONFIG_HPP

#include <cstdint>
#include <string>

#include <vix/config/Config.hpp>

namespace softadastra::cloud::config
{
  /**
   * @brief Application-level configuration for Softadastra Cloud.
   *
   * CloudConfig wraps the Vix configuration object and exposes only the
   * settings needed by the Softadastra Cloud backend.
   *
   * It keeps application code independent from raw environment key names.
   */
  struct CloudConfig
  {
    /**
     * @brief Public service name used in logs and health responses.
     */
    std::string service_name{"softadastra-cloud"};

    /**
     * @brief Current runtime environment.
     *
     * Common values:
     * - development
     * - staging
     * - production
     */
    std::string environment{"development"};

    /**
     * @brief HTTP server host.
     */
    std::string server_host{"127.0.0.1"};

    /**
     * @brief HTTP server port.
     */
    std::uint16_t server_port{8080};

    /**
     * @brief Frontend origin allowed by CORS.
     */
    std::string cors_allowed_origin{"http://localhost:5173"};

    /**
     * @brief Whether development-friendly middleware behavior is enabled.
     */
    bool dev_mode{true};

    /**
     * @brief Whether database migrations should run automatically on startup.
     */
    bool auto_migrate{true};

    /**
     * @brief Secret used for dashboard/API tokens.
     */
    std::string jwt_secret{};

    /**
     * @brief Secret used to hash and verify agent API keys.
     */
    std::string agent_api_key_secret{};

    /**
     * @brief Maximum JSON/body size accepted for API write requests.
     */
    std::uint64_t max_request_body_bytes{1024 * 1024};

    /**
     * @brief Builds CloudConfig from a Vix config instance.
     *
     * @param cfg Loaded Vix configuration.
     * @return CloudConfig.
     */
    [[nodiscard]] static CloudConfig from(
        const vix::config::Config &cfg);

    /**
     * @brief Returns true when the config is usable.
     *
     * @return true if the configuration is valid.
     */
    [[nodiscard]] bool is_valid() const noexcept;

    /**
     * @brief Backward-compatible valid alias.
     *
     * @return true if the configuration is valid.
     */
    [[nodiscard]] bool valid() const noexcept
    {
      return is_valid();
    }

    /**
     * @brief Returns true when the environment is production.
     *
     * @return true if environment == production.
     */
    [[nodiscard]] bool is_production() const noexcept;

    /**
     * @brief Returns true when the environment is development.
     *
     * @return true if environment == development.
     */
    [[nodiscard]] bool is_development() const noexcept;
  };

} // namespace softadastra::cloud::config

#endif // SOFTADASTRA_CLOUD_CONFIG_CLOUD_CONFIG_HPP
