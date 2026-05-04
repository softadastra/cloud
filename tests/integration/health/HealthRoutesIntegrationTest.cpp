/**
 *
 *  @file HealthRoutesIntegrationTest.cpp
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

#include <memory>
#include <string>

#include <vix.hpp>
#include <vix/db/db.hpp>
#include <vix/http/Response.hpp>
#include <vix/tests/tests.hpp>

#include "app/AppState.hpp"
#include "config/CloudConfig.hpp"
#include "modules/health/HealthController.hpp"

namespace
{
  [[nodiscard]] softadastra::cloud::config::CloudConfig make_config()
  {
    softadastra::cloud::config::CloudConfig config;

    config.service_name = "softadastra-cloud-test";
    config.environment = "test";
    config.server_host = "127.0.0.1";
    config.server_port = 8080;
    config.cors_allowed_origin = "http://localhost:5173";
    config.dev_mode = true;
    config.auto_migrate = false;
    config.jwt_secret = "test-jwt-secret";
    config.agent_api_key_secret = "test-agent-secret";
    config.max_request_body_bytes = 1024 * 1024;

    return config;
  }

  [[nodiscard]] softadastra::cloud::app::AppState make_state()
  {
    softadastra::cloud::app::AppState state;

    state.config = make_config();
    state.database =
        std::make_shared<vix::db::Database>(
            vix::db::Database::sqlite(":memory:"));

    return state;
  }

  struct TestResponse
  {
    vix::http::Response native{};
    vix::Response wrapped;

    TestResponse()
        : native(),
          wrapped(native)
    {
    }
  };

  void register_health_routes_integration_tests()
  {
    vix::tests::TestSuite suite{"integration.health.HealthRoutes"};

    suite.add(
        vix::tests::TestCase{
            "health controller returns 200",
            []
            {
              auto state = make_state();

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::health::HealthController::health(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "health endpoint should return 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "health endpoint should return a JSON body");

              vix::tests::Assert::equal(
                  std::string{"application/json; charset=utf-8"},
                  response.native.header("Content-Type"),
                  "health endpoint should return JSON");
            }});

    suite.add(
        vix::tests::TestCase{
            "health response contains service name",
            []
            {
              auto state = make_state();

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::health::HealthController::health(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::is_true(
                  response.native.body().find("softadastra-cloud-test") !=
                      std::string::npos,
                  "health response should contain configured service name");
            }});

    suite.add(
        vix::tests::TestCase{
            "health response contains environment",
            []
            {
              auto state = make_state();

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::health::HealthController::health(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::is_true(
                  response.native.body().find("test") != std::string::npos,
                  "health response should contain configured environment");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_health_routes_integration_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
