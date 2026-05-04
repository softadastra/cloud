/*
 * CloudConfig.cpp
 */

#include "config/CloudConfig.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace softadastra::cloud::config
{
  namespace
  {
    [[nodiscard]] std::string normalize_environment(std::string value)
    {
      std::transform(
          value.begin(),
          value.end(),
          value.begin(),
          [](unsigned char c)
          {
            return static_cast<char>(std::tolower(c));
          });

      if (value.empty())
      {
        return "development";
      }

      return value;
    }

    [[nodiscard]] bool read_bool(
        const vix::config::Config &cfg,
        const std::string &key,
        bool fallback)
    {
      const std::string value =
          cfg.getString(key, fallback ? "true" : "false");

      if (value == "true" ||
          value == "1" ||
          value == "yes" ||
          value == "on")
      {
        return true;
      }

      if (value == "false" ||
          value == "0" ||
          value == "no" ||
          value == "off")
      {
        return false;
      }

      return fallback;
    }

    [[nodiscard]] std::uint16_t read_port(
        const vix::config::Config &cfg)
    {
      const int port =
          cfg.getInt("server.port", cfg.getServerPort());

      if (port <= 0 || port > 65535)
      {
        return 8080;
      }

      return static_cast<std::uint16_t>(port);
    }

    [[nodiscard]] std::uint64_t read_u64(
        const vix::config::Config &cfg,
        const std::string &key,
        std::uint64_t fallback)
    {
      const int value =
          cfg.getInt(key, static_cast<int>(fallback));

      if (value <= 0)
      {
        return fallback;
      }

      return static_cast<std::uint64_t>(value);
    }
  }

  CloudConfig CloudConfig::from(const vix::config::Config &cfg)
  {
    CloudConfig config;

    config.service_name =
        cfg.getString("cloud.service_name", "softadastra-cloud");

    config.environment =
        normalize_environment(
            cfg.getString("cloud.environment", "development"));

    config.server_host =
        cfg.getString("server.host", "127.0.0.1");

    config.server_port = read_port(cfg);

    config.cors_allowed_origin =
        cfg.getString(
            "cloud.cors_allowed_origin",
            "http://localhost:5173");

    config.dev_mode =
        read_bool(
            cfg,
            "cloud.dev_mode",
            !config.is_production());

    config.auto_migrate =
        read_bool(
            cfg,
            "cloud.auto_migrate",
            true);

    config.jwt_secret =
        cfg.getString("cloud.jwt_secret", "");

    config.agent_api_key_secret =
        cfg.getString("cloud.agent_api_key_secret", "");

    config.max_request_body_bytes =
        read_u64(
            cfg,
            "cloud.max_request_body_bytes",
            1024 * 1024);

    return config;
  }

  bool CloudConfig::is_valid() const noexcept
  {
    if (service_name.empty())
    {
      return false;
    }

    if (environment.empty())
    {
      return false;
    }

    if (server_host.empty())
    {
      return false;
    }

    if (server_port == 0)
    {
      return false;
    }
    if (cors_allowed_origin.empty())
    {
      return false;
    }

    if (is_production())
    {
      if (jwt_secret.empty())
      {
        return false;
      }

      if (agent_api_key_secret.empty())
      {
        return false;
      }
    }

    return true;
  }

  bool CloudConfig::is_production() const noexcept
  {
    return environment == "production";
  }

  bool CloudConfig::is_development() const noexcept
  {
    return environment == "development";
  }

} // namespace softadastra::cloud::config
