/**
 *
 *  @file ProjectRoutesIntegrationTest.cpp
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
#include <vix/http/Status.hpp>
#include <vix/tests/tests.hpp>

#include "app/AppState.hpp"
#include "config/CloudConfig.hpp"
#include "database/Migrations.hpp"
#include "modules/projects/ProjectController.hpp"
#include "modules/projects/ProjectRoutes.hpp"
#include "modules/projects/ProjectService.hpp"

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

    softadastra::cloud::database::Migrations::run(*state.database);

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

  void seed_project(
      const softadastra::cloud::app::AppState &state,
      const std::string &name)
  {
    softadastra::cloud::modules::projects::ProjectService service{state};

    const auto result =
        service.create_project(
            name,
            "Seeded project",
            "https://seed.example.com");

    vix::tests::Assert::is_true(
        result.success,
        "seed project should be created");
  }

  void register_project_routes_integration_tests()
  {
    vix::tests::TestSuite suite{"integration.projects.ProjectRoutes"};

    suite.add(
        vix::tests::TestCase{
            "project routes register without throwing",
            []
            {
              auto state = make_state();

              vix::App app;

              softadastra::cloud::modules::projects::ProjectRoutes::register_routes(
                  app,
                  state);

              vix::tests::Assert::is_true(
                  true,
                  "ProjectRoutes::register_routes should not throw");
            }});

    suite.add(
        vix::tests::TestCase{
            "project list endpoint returns 200",
            []
            {
              auto state = make_state();

              seed_project(
                  state,
                  "Integration Project");

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::projects::ProjectController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "project list endpoint should return 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "project list endpoint should return a body");

              vix::tests::Assert::equal(
                  std::string{"application/json; charset=utf-8"},
                  response.native.header("Content-Type"),
                  "project list endpoint should return JSON");
            }});

    suite.add(
        vix::tests::TestCase{
            "project list endpoint contains seeded project",
            []
            {
              auto state = make_state();

              seed_project(
                  state,
                  "Seeded Route Project");

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::projects::ProjectController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::is_true(
                  response.native.body().find("Seeded Route Project") !=
                      std::string::npos,
                  "response body should contain seeded project name");

              vix::tests::Assert::is_true(
                  response.native.body().find("seeded-route-project") !=
                      std::string::npos,
                  "response body should contain generated project slug");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_project_routes_integration_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
