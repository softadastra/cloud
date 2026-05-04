/**
 *
 *  @file RunValidatorTest.cpp
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

#include "validation/RunValidator.hpp"

namespace
{
  void register_run_validator_tests()
  {
    vix::tests::TestSuite suite{"validation.RunValidator"};

    suite.add(
        vix::tests::TestCase{
            "valid target URL passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_target_url(
                      "https://api.example.com/orders");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "https target URL should be valid");
            }});

    suite.add(
        vix::tests::TestCase{
            "http target URL passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_target_url(
                      "http://localhost:8080/health");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "http target URL should be valid");
            }});

    suite.add(
        vix::tests::TestCase{
            "empty target URL fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_target_url(
                      "");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "empty target URL should fail");

              vix::tests::Assert::equal(
                  std::string{"target_url"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"required"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "target URL without protocol fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_target_url(
                      "api.example.com/orders");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "target URL without protocol should fail");

              vix::tests::Assert::equal(
                  std::string{"invalid_url"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "valid HTTP methods pass",
            []
            {
              const char *methods[] = {
                  "GET",
                  "POST",
                  "PUT",
                  "PATCH",
                  "DELETE",
                  "HEAD",
                  "OPTIONS",
              };

              for (const char *method : methods)
              {
                const auto error =
                    softadastra::cloud::validation::RunValidator::validate_method(
                        method);

                vix::tests::Assert::is_false(
                    error.has_value(),
                    std::string{"method should be valid: "} + method);
              }
            }});

    suite.add(
        vix::tests::TestCase{
            "empty method defaults to GET",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_method(
                      "");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "empty method should default to GET");
            }});

    suite.add(
        vix::tests::TestCase{
            "lowercase method is accepted after normalization",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_method(
                      "post");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "lowercase method should be normalized");
            }});

    suite.add(
        vix::tests::TestCase{
            "invalid method fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_method(
                      "CONNECT");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "invalid method should fail");

              vix::tests::Assert::equal(
                  std::string{"method"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"invalid_method"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "valid scenario passes",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_scenario(
                      "unstable_network");

              vix::tests::Assert::is_false(
                  error.has_value(),
                  "valid scenario should pass");
            }});

    suite.add(
        vix::tests::TestCase{
            "empty scenario fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_scenario(
                      "");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "empty scenario should fail");

              vix::tests::Assert::equal(
                  std::string{"scenario"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"required"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "too short scenario fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_scenario(
                      "x");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "too short scenario should fail");

              vix::tests::Assert::equal(
                  std::string{"invalid_length"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "valid statuses pass",
            []
            {
              const char *statuses[] = {
                  "created",
                  "running",
                  "finished",
                  "failed",
                  "cancelled",
              };

              for (const char *status : statuses)
              {
                const auto error =
                    softadastra::cloud::validation::RunValidator::validate_status(
                        status);

                vix::tests::Assert::is_false(
                    error.has_value(),
                    std::string{"status should be valid: "} + status);
              }
            }});

    suite.add(
        vix::tests::TestCase{
            "invalid status fails",
            []
            {
              const auto error =
                  softadastra::cloud::validation::RunValidator::validate_status(
                      "done");

              vix::tests::Assert::is_true(
                  error.has_value(),
                  "invalid status should fail");

              vix::tests::Assert::equal(
                  std::string{"status"},
                  error->field);

              vix::tests::Assert::equal(
                  std::string{"invalid_status"},
                  error->code);
            }});

    suite.add(
        vix::tests::TestCase{
            "final statuses are detected",
            []
            {
              vix::tests::Assert::is_true(
                  softadastra::cloud::validation::RunValidator::is_final_status(
                      "finished"));

              vix::tests::Assert::is_true(
                  softadastra::cloud::validation::RunValidator::is_final_status(
                      "failed"));

              vix::tests::Assert::is_true(
                  softadastra::cloud::validation::RunValidator::is_final_status(
                      "cancelled"));

              vix::tests::Assert::is_false(
                  softadastra::cloud::validation::RunValidator::is_final_status(
                      "running"));
            }});

    suite.add(
        vix::tests::TestCase{
            "normalize_method uppercases method",
            []
            {
              const std::string method =
                  softadastra::cloud::validation::RunValidator::normalize_method(
                      "patch");

              vix::tests::Assert::equal(
                  std::string{"PATCH"},
                  method);
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_run_validator_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
