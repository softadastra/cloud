/**
 *
 *  @file EventValidatorTest.cpp
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

#include <optional>
#include <string>

#include <vix/tests/tests.hpp>

#include "validation/EventValidator.hpp"

namespace
{
  void register_event_validator_tests()
  {
    vix::tests::TestSuite suite{"validation.EventValidator"};

    suite.add(
        vix::tests::TestCase{
            "valid event type passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_event_type(
                      "request_timeout");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "valid event type should pass");
            }});

    suite.add(
        vix::tests::TestCase{
            "empty event type fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_event_type(
                      "");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "empty event type should fail");

              vix::tests::Assert::equal(
                  std::string{"event_type"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"required"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "too short event type fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_event_type(
                      "x");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "too short event type should fail");

              vix::tests::Assert::equal(
                  std::string{"invalid_length"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "valid severities pass",
            []
            {
              const char *severities[] = {
                  "info",
                  "warning",
                  "warn",
                  "error",
                  "critical",
                  "INFO",
                  "WARNING",
                  "ERROR",
                  "CRITICAL",
              };

              for (const char *severity : severities)
              {
                const auto error =
                    softadastra::cloud::validation::EventValidator::validate_severity(
                        severity);

                vix::tests::Assert::is_false(
                    error.has_value(),
                    std::string{"severity should be valid: "} + severity);
              }
            }});

    suite.add(
        vix::tests::TestCase{
            "empty severity defaults to info",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_severity(
                      "");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "empty severity should default to info");
            }});

    suite.add(
        vix::tests::TestCase{
            "invalid severity fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_severity(
                      "fatal");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "invalid severity should fail");

              vix::tests::Assert::equal(
                  std::string{"severity"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"invalid_severity"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "message below limit passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_message(
                      "Request timed out after 5000ms");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "normal message should pass");
            }});

    suite.add(
        vix::tests::TestCase{
            "empty message passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_message(
                      "");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "empty message should be allowed");
            }});

    suite.add(
        vix::tests::TestCase{
            "too long message fails",
            []
            {
              const std::string message(1001, 'x');

              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_message(
                      message);

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "too long message should fail");

              vix::tests::Assert::equal(
                  std::string{"message"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"too_long"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "payload below limit passes",
            []
            {
              const std::string payload =
                  R"({"status":500,"duration_ms":5020})";

              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_payload(
                      payload);

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "normal payload should pass");
            }});

    suite.add(
        vix::tests::TestCase{
            "empty payload passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_payload(
                      "");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "empty payload should be allowed");
            }});

    suite.add(
        vix::tests::TestCase{
            "too large payload fails",
            []
            {
              const std::string payload((1024 * 1024) + 1, 'x');

              const auto error =
                  softadastra::cloud::validation::EventValidator::validate_payload(
                      payload);

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "too large payload should fail");

              vix::tests::Assert::equal(
                  std::string{"payload"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"too_large"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "normalize_severity lowercases severity",
            []
            {
              const std::string severity =
                  softadastra::cloud::validation::EventValidator::normalize_severity(
                      "ERROR");

              vix::tests::Assert::equal(
                  std::string{"error"},
                  severity);
            }});

    suite.add(
        vix::tests::TestCase{
            "normalize_severity converts warn to warning",
            []
            {
              const std::string severity =
                  softadastra::cloud::validation::EventValidator::normalize_severity(
                      "warn");

              vix::tests::Assert::equal(
                  std::string{"warning"},
                  severity);
            }});

    suite.add(
        vix::tests::TestCase{
            "normalize_severity defaults empty value to info",
            []
            {
              const std::string severity =
                  softadastra::cloud::validation::EventValidator::normalize_severity(
                      "");

              vix::tests::Assert::equal(
                  std::string{"info"},
                  severity);
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_event_validator_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
