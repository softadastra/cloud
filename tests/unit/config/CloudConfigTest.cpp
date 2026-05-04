/**
 *
 *  @file CloudConfigTest.cpp
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

#include <vix/config/Config.hpp>
#include <vix/tests/tests.hpp>

#include "config/CloudConfig.hpp"

namespace
{
  [[nodiscard]] softadastra::cloud::config::CloudConfig make_valid_config()
  {
    softadastra::cloud::config::CloudConfig config;

    config.service_name = "softadastra-cloud-test";
    config.environment = "development";
    config.server_host = "127.0.0.1";
    config.server_port = 8080;
    config.cors_allowed_origin = "http://localhost:5173";
    config.dev_mode = true;
    config.auto_migrate = true;
    config.jwt_secret = "";
    config.agent_api_key_secret = "";
    config.max_request_body_bytes = 1024 * 1024;

    return config;
  }

  void register_cloud_config_tests()
  {
    vix::tests::TestSuite suite{"config.CloudConfig"};

    suite.add(
        vix::tests::TestCase{
            "default config is valid in development",
            []
            {
              const softadastra::cloud::config::CloudConfig config;

              vix::tests::Assert::is_true(
                  config.is_valid(),
                  "default development config should be valid");
            }});

    suite.add(
        vix::tests::TestCase{
            "valid alias returns same result as is_valid",
            []
            {
              const auto config = make_valid_config();

              vix::tests::Assert::equal(
                  config.is_valid(),
                  config.valid(),
                  "valid() should match is_valid()");
            }});

    suite.add(
        vix::tests::TestCase{
            "config with empty service name is invalid",
            []
            {
              auto config = make_valid_config();
              config.service_name.clear();

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "empty service name should be invalid");
            }});

    suite.add(
        vix::tests::TestCase{
            "config with empty environment is invalid",
            []
            {
              auto config = make_valid_config();
              config.environment.clear();

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "empty environment should be invalid");
            }});

    suite.add(
        vix::tests::TestCase{
            "config with empty server host is invalid",
            []
            {
              auto config = make_valid_config();
              config.server_host.clear();

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "empty server host should be invalid");
            }});

    suite.add(
        vix::tests::TestCase{
            "config with zero server port is invalid",
            []
            {
              auto config = make_valid_config();
              config.server_port = 0;

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "zero server port should be invalid");
            }});

    suite.add(
        vix::tests::TestCase{
            "config with empty CORS origin is invalid",
            []
            {
              auto config = make_valid_config();
              config.cors_allowed_origin.clear();

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "empty CORS origin should be invalid");
            }});

    suite.add(
        vix::tests::TestCase{
            "production config requires jwt secret",
            []
            {
              auto config = make_valid_config();
              config.environment = "production";
              config.jwt_secret.clear();
              config.agent_api_key_secret = "agent-secret";

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "production config should require jwt_secret");
            }});

    suite.add(
        vix::tests::TestCase{
            "production config requires agent api key secret",
            []
            {
              auto config = make_valid_config();
              config.environment = "production";
              config.jwt_secret = "jwt-secret";
              config.agent_api_key_secret.clear();

              vix::tests::Assert::is_false(
                  config.is_valid(),
                  "production config should require agent_api_key_secret");
            }});

    suite.add(
        vix::tests::TestCase{
            "production config is valid with required secrets",
            []
            {
              auto config = make_valid_config();
              config.environment = "production";
              config.jwt_secret = "jwt-secret";
              config.agent_api_key_secret = "agent-secret";

              vix::tests::Assert::is_true(
                  config.is_valid(),
                  "production config should be valid when secrets are present");
            }});

    suite.add(
        vix::tests::TestCase{
            "is_production detects production environment",
            []
            {
              auto config = make_valid_config();
              config.environment = "production";

              vix::tests::Assert::is_true(
                  config.is_production(),
                  "environment=production should be production");

              vix::tests::Assert::is_false(
                  config.is_development(),
                  "environment=production should not be development");
            }});

    suite.add(
        vix::tests::TestCase{
            "is_development detects development environment",
            []
            {
              auto config = make_valid_config();
              config.environment = "development";

              vix::tests::Assert::is_true(
                  config.is_development(),
                  "environment=development should be development");

              vix::tests::Assert::is_false(
                  config.is_production(),
                  "environment=development should not be production");
            }});

    suite.add(
        vix::tests::TestCase{
            "from loads defaults from empty vix config",
            []
            {
              const vix::config::Config raw_config;
              const auto config =
                  softadastra::cloud::config::CloudConfig::from(raw_config);

              vix::tests::Assert::equal(
                  std::string{"softadastra-cloud"},
                  config.service_name);

              vix::tests::Assert::equal(
                  std::string{"development"},
                  config.environment);

              vix::tests::Assert::equal(
                  std::string{"127.0.0.1"},
                  config.server_host);

              vix::tests::Assert::equal(
                  static_cast<std::uint16_t>(8080),
                  config.server_port);

              vix::tests::Assert::equal(
                  std::string{"http://localhost:5173"},
                  config.cors_allowed_origin);

              vix::tests::Assert::is_true(
                  config.dev_mode,
                  "development config should enable dev_mode by default");

              vix::tests::Assert::is_true(
                  config.auto_migrate,
                  "auto_migrate should be true by default");

              vix::tests::Assert::equal(
                  static_cast<std::uint64_t>(1024 * 1024),
                  config.max_request_body_bytes);
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_cloud_config_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
