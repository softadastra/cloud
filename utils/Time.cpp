/*
 * Time.cpp
 */

#include "utils/Time.hpp"

#include <chrono>
#include <string>

#include <vix/time/time.hpp>

namespace softadastra::cloud::utils
{
  std::int64_t Time::now_ms()
  {
    using namespace std::chrono;

    return static_cast<std::int64_t>(
        duration_cast<milliseconds>(
            system_clock::now().time_since_epoch())
            .count());
  }

  std::int64_t Time::now_seconds()
  {
    return static_cast<std::int64_t>(
        vix::time::Timestamp::now().seconds_since_epoch());
  }

  std::string Time::now_utc_string()
  {
    return vix::time::DateTime::now_utc().to_string_utc();
  }

  std::int64_t Time::ms_to_seconds(
      std::int64_t millis) noexcept
  {
    return millis / 1000;
  }

  std::int64_t Time::seconds_to_ms(
      std::int64_t seconds) noexcept
  {
    return seconds * 1000;
  }

} // namespace softadastra::cloud::utils
