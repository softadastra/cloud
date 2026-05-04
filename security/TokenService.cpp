/*
 * TokenService.cpp
 */

#include "security/TokenService.hpp"

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/Time.hpp"

namespace softadastra::cloud::security
{
  namespace
  {
    [[nodiscard]] std::string hash_payload(
        const std::string &secret,
        const std::string &payload)
    {
      const std::hash<std::string> hasher;
      const auto value =
          hasher(secret + ":" + payload);

      std::ostringstream out;
      out << "v1$" << value;

      return out.str();
    }

    [[nodiscard]] std::vector<std::string> split(
        const std::string &value,
        char delimiter)
    {
      std::vector<std::string> parts;
      std::string current;

      for (const char c : value)
      {
        if (c == delimiter)
        {
          parts.push_back(current);
          current.clear();
          continue;
        }

        current.push_back(c);
      }

      parts.push_back(current);

      return parts;
    }
  }

  std::string TokenService::issue_user_token(
      const softadastra::cloud::config::CloudConfig &config,
      const std::string &user_public_id,
      const std::string &email)
  {
    if (config.jwt_secret.empty())
    {
      throw std::invalid_argument(
          "jwt_secret is not configured");
    }

    if (user_public_id.empty())
    {
      throw std::invalid_argument(
          "user_public_id cannot be empty");
    }

    if (email.empty())
    {
      throw std::invalid_argument(
          "email cannot be empty");
    }

    const std::string issued_at =
        std::to_string(
            softadastra::cloud::utils::Time::now_ms());

    const std::string payload =
        user_public_id +
        "." +
        email +
        "." +
        issued_at;

    const std::string signature =
        sign(config, payload);

    return "v1." + payload + "." + signature;
  }

  bool TokenService::verify_user_token(
      const softadastra::cloud::config::CloudConfig &config,
      const std::string &token)
  {
    if (config.jwt_secret.empty() || token.empty())
    {
      return false;
    }

    const auto parts = split(token, '.');

    if (parts.size() != 5)
    {
      return false;
    }

    if (parts[0] != "v1")
    {
      return false;
    }

    const std::string payload =
        parts[1] +
        "." +
        parts[2] +
        "." +
        parts[3];

    const std::string expected_signature =
        sign(config, payload);

    return constant_time_equals(
        expected_signature,
        parts[4]);
  }

  std::string TokenService::sign(
      const softadastra::cloud::config::CloudConfig &config,
      const std::string &payload)
  {
    if (payload.empty())
    {
      throw std::invalid_argument(
          "token payload cannot be empty");
    }

    return hash_payload(
        config.jwt_secret,
        payload);
  }

  bool TokenService::constant_time_equals(
      const std::string &a,
      const std::string &b) noexcept
  {
    if (a.size() != b.size())
    {
      return false;
    }

    unsigned char diff = 0;

    for (std::size_t i = 0; i < a.size(); ++i)
    {
      diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }

    return diff == 0;
  }

} // namespace softadastra::cloud::security
