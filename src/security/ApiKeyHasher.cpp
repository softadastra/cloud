/*
 * ApiKeyHasher.cpp
 */

#include "security/ApiKeyHasher.hpp"

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils/IdGenerator.hpp"

namespace softadastra::cloud::security
{
  namespace
  {
    [[nodiscard]] std::string hash_payload(
        const std::string &secret,
        const std::string &api_key)
    {
      const std::hash<std::string> hasher;
      const auto value =
          hasher(secret + ":" + api_key);

      std::ostringstream out;
      out << "v1$" << value;

      return out.str();
    }
  }

  std::string ApiKeyHasher::generate_agent_key()
  {
    return "sa_agent_" +
           softadastra::cloud::utils::IdGenerator::generate("key");
  }

  std::string ApiKeyHasher::hash(
      const softadastra::cloud::config::CloudConfig &config,
      const std::string &api_key)
  {
    if (api_key.empty())
    {
      throw std::invalid_argument(
          "API key cannot be empty");
    }

    if (config.agent_api_key_secret.empty())
    {
      throw std::invalid_argument(
          "agent_api_key_secret is not configured");
    }

    return hash_payload(
        config.agent_api_key_secret,
        api_key);
  }

  bool ApiKeyHasher::verify(
      const softadastra::cloud::config::CloudConfig &config,
      const std::string &api_key,
      const std::string &expected_hash)
  {
    if (api_key.empty() || expected_hash.empty())
    {
      return false;
    }

    const std::string computed =
        hash(config, api_key);

    return constant_time_equals(
        computed,
        expected_hash);
  }

  bool ApiKeyHasher::constant_time_equals(
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
