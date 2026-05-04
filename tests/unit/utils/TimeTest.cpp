/**
 *
 *  @file TimeTest.cpp
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

#include <cstdint>
#include <string>

#include <vix/tests/tests.hpp>

#include "utils/Time.hpp"

namespace
{
  void register_time_tests()
  {
    vix::tests::TestSuite suite{"utils.Time"};

    suite.add(
        vix::tests::TestCase{
            "now_ms returns positive timestamp",
            []
            {
              const std::int64_t now =
                  softadastra::cloud::utils::Time::now_ms();

              vix::tests::Assert::is_true(
                  now > 0,
                  "now_ms should return a positive timestamp");
            }});

    suite.add(
        vix::tests::TestCase{
            "now_seconds returns positive timestamp",
            []
            {
              const std::int64_t now =
                  softadastra::cloud::utils::Time::now_seconds();

              vix::tests::Assert::is_true(
                  now > 0,
                  "now_seconds should return a positive timestamp");
            }});

    suite.add(
        vix::tests::TestCase{
            "now_ms is compatible with now_seconds",
            []
            {
              const std::int64_t millis =
                  softadastra::cloud::utils::Time::now_ms();

              const std::int64_t seconds =
                  softadastra::cloud::utils::Time::now_seconds();

              vix::tests::Assert::is_true(
                  millis > seconds,
                  "milliseconds timestamp should be larger than seconds timestamp");

              vix::tests::Assert::is_true(
                  millis / 1000 >= seconds - 1,
                  "milliseconds and seconds should represent the same current time window");
            }});

    suite.add(
        vix::tests::TestCase{
            "ms_to_seconds converts milliseconds to seconds",
            []
            {
              vix::tests::Assert::equal(
                  static_cast<std::int64_t>(1),
                  softadastra::cloud::utils::Time::ms_to_seconds(1000));

              vix::tests::Assert::equal(
                  static_cast<std::int64_t>(2),
                  softadastra::cloud::utils::Time::ms_to_seconds(2500));

              vix::tests::Assert::equal(
                  static_cast<std::int64_t>(0),
                  softadastra::cloud::utils::Time::ms_to_seconds(0));
            }});

    suite.add(
        vix::tests::TestCase{
            "seconds_to_ms converts seconds to milliseconds",
            []
            {
              vix::tests::Assert::equal(
                  static_cast<std::int64_t>(1000),
                  softadastra::cloud::utils::Time::seconds_to_ms(1));

              vix::tests::Assert::equal(
                  static_cast<std::int64_t>(2000),
                  softadastra::cloud::utils::Time::seconds_to_ms(2));

              vix::tests::Assert::equal(
                  static_cast<std::int64_t>(0),
                  softadastra::cloud::utils::Time::seconds_to_ms(0));
            }});

    suite.add(
        vix::tests::TestCase{
            "now_utc_string returns non-empty string",
            []
            {
              const std::string value =
                  softadastra::cloud::utils::Time::now_utc_string();

              vix::tests::Assert::is_false(
                  value.empty(),
                  "now_utc_string should not be empty");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_time_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
