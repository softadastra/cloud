/**
 *
 *  @file TokenServiceTest.cpp
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

#include <stdexcept>
#include <string>

#include <vix/tests/tests.hpp>

#include "config/CloudConfig.hpp"
#include "security/TokenService.hpp"

namespace
{
  [[nodiscard]] softadastra::cloud::config::CloudConfig make_config()
  {
    softadastra::cloud::config::CloudConfig config;

    config.service_name = "softadastra-cloud-test";
    config.environment = "test";
    config.agent_api_key_secret = "test-agent-secret";
    config.jwt_secret = "test-jwt-secret";
    config.server_host = "127.0.0.1";
    config.server_port = 8080;
    config.cors_allowed_origin = "http://localhost:5173";
    config.max_request_body_bytes = 1024 * 1024;

    return config;
  }

  void register_token_service_tests()
  {
    vix::tests::TestSuite suite{"security.TokenService"};

    suite.add(
        vix::tests::TestCase{
            "issue_user_token returns v1 token",
            []
            {
              const auto config = make_config();

              const std::string token =
                  softadastra::cloud::security::TokenService::issue_user_token(
                      config,
                      "usr_test",
                      "user@example.com");

              vix::tests::Assert::is_true(
                  token.rfind("v1.", 0) == 0,
                  "token should start with v1.");
            }});

    suite.add(
        vix::tests::TestCase{
            "issued token verifies successfully",
            []
            {
              const auto config = make_config();

              const std::string token =
                  softadastra::cloud::security::TokenService::issue_user_token(
                      config,
                      "usr_test",
                      "user@example.com");

              const bool verified =
                  softadastra::cloud::security::TokenService::verify_user_token(
                      config,
                      token);

              vix::tests::Assert::is_true(
                  verified,
                  "freshly issued token should verify");
            }});

    suite.add(
        vix::tests::TestCase{
            "token does not verify with different secret",
            []
            {
              const auto first_config = make_config();

              auto second_config = make_config();
              second_config.jwt_secret = "another-jwt-secret";

              const std::string token =
                  softadastra::cloud::security::TokenService::issue_user_token(
                      first_config,
                      "usr_test",
                      "user@example.com");

              const bool verified =
                  softadastra::cloud::security::TokenService::verify_user_token(
                      second_config,
                      token);

              vix::tests::Assert::is_false(
                  verified,
                  "token should not verify with a different secret");
            }});

    suite.add(
        vix::tests::TestCase{
            "tampered token does not verify",
            []
            {
              const auto config = make_config();

              std::string token =
                  softadastra::cloud::security::TokenService::issue_user_token(
                      config,
                      "usr_test",
                      "user@example.com");

              token += "tampered";

              const bool verified =
                  softadastra::cloud::security::TokenService::verify_user_token(
                      config,
                      token);

              vix::tests::Assert::is_false(
                  verified,
                  "tampered token should not verify");
            }});

    suite.add(
        vix::tests::TestCase{
            "empty token does not verify",
            []
            {
              const auto config = make_config();

              const bool verified =
                  softadastra::cloud::security::TokenService::verify_user_token(
                      config,
                      "");

              vix::tests::Assert::is_false(
                  verified,
                  "empty token should not verify");
            }});

    suite.add(
        vix::tests::TestCase{
            "invalid token format does not verify",
            []
            {
              const auto config = make_config();

              const bool verified =
                  softadastra::cloud::security::TokenService::verify_user_token(
                      config,
                      "invalid.token.format");

              vix::tests::Assert::is_false(
                  verified,
                  "invalid token format should not verify");
            }});

    suite.add(
        vix::tests::TestCase{
            "issue_user_token throws when jwt secret is missing",
            []
            {
              auto config = make_config();
              config.jwt_secret.clear();

              bool thrown = false;

              try
              {
                static_cast<void>(
                    softadastra::cloud::security::TokenService::issue_user_token(
                        config,
                        "usr_test",
                        "user@example.com"));
              }
              catch (const std::invalid_argument &)
              {
                thrown = true;
              }

              vix::tests::Assert::is_true(
                  thrown,
                  "missing jwt secret should throw invalid_argument");
            }});

    suite.add(
        vix::tests::TestCase{
            "issue_user_token throws when user id is empty",
            []
            {
              const auto config = make_config();

              bool thrown = false;

              try
              {
                static_cast<void>(
                    softadastra::cloud::security::TokenService::issue_user_token(
                        config,
                        "",
                        "user@example.com"));
              }
              catch (const std::invalid_argument &)
              {
                thrown = true;
              }

              vix::tests::Assert::is_true(
                  thrown,
                  "empty user id should throw invalid_argument");
            }});

    suite.add(
        vix::tests::TestCase{
            "issue_user_token throws when email is empty",
            []
            {
              const auto config = make_config();

              bool thrown = false;

              try
              {
                static_cast<void>(
                    softadastra::cloud::security::TokenService::issue_user_token(
                        config,
                        "usr_test",
                        ""));
              }
              catch (const std::invalid_argument &)
              {
                thrown = true;
              }

              vix::tests::Assert::is_true(
                  thrown,
                  "empty email should throw invalid_argument");
            }});

    suite.add(
        vix::tests::TestCase{
            "verify returns false when jwt secret is missing",
            []
            {
              auto config = make_config();

              const std::string token =
                  softadastra::cloud::security::TokenService::issue_user_token(
                      config,
                      "usr_test",
                      "user@example.com");

              config.jwt_secret.clear();

              const bool verified =
                  softadastra::cloud::security::TokenService::verify_user_token(
                      config,
                      token);

              vix::tests::Assert::is_false(
                  verified,
                  "token should not verify when jwt secret is missing");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_token_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
