/*
 * IdGenerator.cpp
 */

#include "utils/IdGenerator.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstddef>
#include <random>
#include <string>
#include <string_view>

namespace softadastra::cloud::utils
{
  namespace
  {
    [[nodiscard]] std::mt19937_64 &rng() noexcept
    {
      thread_local std::mt19937_64 engine(
          []
          {
            std::random_device rd;

            const auto now =
                static_cast<std::uint64_t>(
                    std::chrono::high_resolution_clock::now()
                        .time_since_epoch()
                        .count());

            const std::uint64_t seed =
                (static_cast<std::uint64_t>(rd()) << 32) ^ now;

            return seed;
          }());

      return engine;
    }

    [[nodiscard]] char hex_digit(std::uint8_t value) noexcept
    {
      static constexpr char digits[] = "0123456789abcdef";
      return digits[value & 0x0F];
    }

    [[nodiscard]] std::string random_hex(std::size_t bytes)
    {
      std::uniform_int_distribution<std::uint32_t> dist(0, 255);

      std::string out;
      out.reserve(bytes * 2);

      for (std::size_t i = 0; i < bytes; ++i)
      {
        const auto byte =
            static_cast<std::uint8_t>(dist(rng()));

        out.push_back(hex_digit(byte >> 4));
        out.push_back(hex_digit(byte));
      }

      return out;
    }

    [[nodiscard]] bool valid_prefix(std::string_view prefix) noexcept
    {
      if (prefix.empty() || prefix.size() > 16)
      {
        return false;
      }

      for (const char c : prefix)
      {
        const bool is_lower =
            c >= 'a' && c <= 'z';

        const bool is_digit =
            c >= '0' && c <= '9';

        if (!is_lower && !is_digit)
        {
          return false;
        }
      }

      return true;
    }
  }

  std::string IdGenerator::generate(
      std::string_view prefix)
  {
    const std::string safe_prefix =
        valid_prefix(prefix)
            ? std::string(prefix)
            : std::string("id");

    return safe_prefix + "_" + random_suffix();
  }

  std::string IdGenerator::random_suffix()
  {
    /*
     * 16 random bytes -> 32 hex characters.
     * Good enough for public ids and API key suffixes in the MVP.
     */
    return random_hex(16);
  }

} // namespace softadastra::cloud::utils
