/**
 *
 *  @file ApiKeyHasherTest.cpp
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
#include "security/ApiKeyHasher.hpp"

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

  void register_api_key_hasher_tests()
  {
    vix::tests::TestSuite suite{"security.ApiKeyHasher"};

    suite.add(
        vix::tests::TestCase{
            "generate_agent_key returns key with expected prefix",
            []
            {
              const std::string api_key =
                  softadastra::cloud::security::ApiKeyHasher::generate_agent_key();

              vix::tests::Assert::is_true(
                  api_key.rfind("sa_agent_key_", 0) == 0,
                  "agent key should start with sa_agent_key_");
            }});

    suite.add(
        vix::tests::TestCase{
            "generate_agent_key returns different values",
            []
            {
              const std::string first =
                  softadastra::cloud::security::ApiKeyHasher::generate_agent_key();

              const std::string second =
                  softadastra::cloud::security::ApiKeyHasher::generate_agent_key();

              vix::tests::Assert::not_equal(
                  first,
                  second,
                  "generated API keys should be unique");
            }});

    suite.add(
        vix::tests::TestCase{
            "hash returns stable value for same key and secret",
            []
            {
              const auto config = make_config();

              const std::string first =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      config,
                      "sa_agent_example_key");

              const std::string second =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      config,
                      "sa_agent_example_key");

              vix::tests::Assert::equal(
                  first,
                  second,
                  "hash should be stable for same key and secret");
            }});

    suite.add(
        vix::tests::TestCase{
            "hash changes when key changes",
            []
            {
              const auto config = make_config();

              const std::string first =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      config,
                      "sa_agent_first_key");

              const std::string second =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      config,
                      "sa_agent_second_key");

              vix::tests::Assert::not_equal(
                  first,
                  second,
                  "different keys should produce different hashes");
            }});

    suite.add(
        vix::tests::TestCase{
            "hash changes when secret changes",
            []
            {
              auto first_config = make_config();
              auto second_config = make_config();

              second_config.agent_api_key_secret = "another-secret";

              const std::string first =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      first_config,
                      "sa_agent_example_key");

              const std::string second =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      second_config,
                      "sa_agent_example_key");

              vix::tests::Assert::not_equal(
                  first,
                  second,
                  "different secrets should produce different hashes");
            }});

    suite.add(
        vix::tests::TestCase{
            "hash throws when API key is empty",
            []
            {
              const auto config = make_config();

              bool thrown = false;

              try
              {
                static_cast<void>(
                    softadastra::cloud::security::ApiKeyHasher::hash(
                        config,
                        ""));
              }
              catch (const std::invalid_argument &)
              {
                thrown = true;
              }

              vix::tests::Assert::is_true(
                  thrown,
                  "empty API key should throw invalid_argument");
            }});

    suite.add(
        vix::tests::TestCase{
            "hash throws when secret is missing",
            []
            {
              auto config = make_config();
              config.agent_api_key_secret.clear();

              bool thrown = false;

              try
              {
                static_cast<void>(
                    softadastra::cloud::security::ApiKeyHasher::hash(
                        config,
                        "sa_agent_example_key"));
              }
              catch (const std::invalid_argument &)
              {
                thrown = true;
              }

              vix::tests::Assert::is_true(
                  thrown,
                  "missing secret should throw invalid_argument");
            }});

    suite.add(
        vix::tests::TestCase{
            "verify returns true for matching key and hash",
            []
            {
              const auto config = make_config();
              const std::string api_key = "sa_agent_example_key";

              const std::string hash =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      config,
                      api_key);

              const bool verified =
                  softadastra::cloud::security::ApiKeyHasher::verify(
                      config,
                      api_key,
                      hash);

              vix::tests::Assert::is_true(
                  verified,
                  "matching API key and hash should verify");
            }});

    suite.add(
        vix::tests::TestCase{
            "verify returns false for invalid key",
            []
            {
              const auto config = make_config();

              const std::string hash =
                  softadastra::cloud::security::ApiKeyHasher::hash(
                      config,
                      "sa_agent_real_key");

              const bool verified =
                  softadastra::cloud::security::ApiKeyHasher::verify(
                      config,
                      "sa_agent_wrong_key",
                      hash);

              vix::tests::Assert::is_false(
                  verified,
                  "wrong API key should not verify");
            }});

    suite.add(
        vix::tests::TestCase{
            "verify returns false for empty values",
            []
            {
              const auto config = make_config();

              vix::tests::Assert::is_false(
                  softadastra::cloud::security::ApiKeyHasher::verify(
                      config,
                      "",
                      "hash"));

              vix::tests::Assert::is_false(
                  softadastra::cloud::security::ApiKeyHasher::verify(
                      config,
                      "sa_agent_key",
                      ""));
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_api_key_hasher_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
