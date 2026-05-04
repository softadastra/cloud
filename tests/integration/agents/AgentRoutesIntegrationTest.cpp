/**
 *
 *  @file AgentRoutesIntegrationTest.cpp
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
#include "modules/agents/AgentController.hpp"
#include "modules/agents/AgentRoutes.hpp"
#include "modules/agents/AgentService.hpp"
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

  [[nodiscard]] std::string seed_project(
      const softadastra::cloud::app::AppState &state)
  {
    softadastra::cloud::modules::projects::ProjectService service{state};

    const auto result =
        service.create_project(
            "Agent Route Project",
            "Project used by agent route tests",
            "https://agent-route.example.com");

    vix::tests::Assert::is_true(
        result.success,
        "seed project should be created");

    return result.project.public_id;
  }

  void seed_agent(
      const softadastra::cloud::app::AppState &state,
      const std::string &project_id,
      const std::string &name)
  {
    softadastra::cloud::modules::agents::AgentService service{state};

    const auto result =
        service.create_agent(
            project_id,
            name);

    vix::tests::Assert::is_true(
        result.success,
        "seed agent should be created");
  }

  void register_agent_routes_integration_tests()
  {
    vix::tests::TestSuite suite{"integration.agents.AgentRoutes"};

    suite.add(
        vix::tests::TestCase{
            "agent routes register without throwing",
            []
            {
              auto state = make_state();

              vix::App app;

              softadastra::cloud::modules::agents::AgentRoutes::register_routes(
                  app,
                  state);

              vix::tests::Assert::is_true(
                  true,
                  "AgentRoutes::register_routes should not throw");
            }});

    suite.add(
        vix::tests::TestCase{
            "agent list endpoint returns 200",
            []
            {
              auto state = make_state();
              const std::string project_id = seed_project(state);

              seed_agent(
                  state,
                  project_id,
                  "Integration Agent");

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::agents::AgentController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "agent list endpoint should return 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "agent list endpoint should return a body");

              vix::tests::Assert::equal(
                  std::string{"application/json; charset=utf-8"},
                  response.native.header("Content-Type"),
                  "agent list endpoint should return JSON");
            }});

    suite.add(
        vix::tests::TestCase{
            "agent list endpoint contains seeded agent",
            []
            {
              auto state = make_state();
              const std::string project_id = seed_project(state);

              seed_agent(
                  state,
                  project_id,
                  "Seeded Agent");

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::agents::AgentController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::is_true(
                  response.native.body().find("Seeded Agent") !=
                      std::string::npos,
                  "response body should contain seeded agent name");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_agent_routes_integration_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
