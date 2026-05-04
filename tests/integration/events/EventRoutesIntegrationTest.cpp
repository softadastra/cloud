/**
 *
 *  @file EventRoutesIntegrationTest.cpp
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
#include "modules/events/EventController.hpp"
#include "modules/events/EventRoutes.hpp"
#include "modules/events/EventService.hpp"
#include "modules/projects/ProjectService.hpp"
#include "modules/runs/RunService.hpp"

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
            "Event Route Project",
            "Project used by event route tests",
            "https://event-route.example.com");

    vix::tests::Assert::is_true(
        result.success,
        "seed project should be created");

    return result.project.public_id;
  }

  [[nodiscard]] std::string seed_run(
      const softadastra::cloud::app::AppState &state)
  {
    const std::string project_id = seed_project(state);

    softadastra::cloud::modules::runs::RunService service{state};

    const auto result =
        service.create_run(
            project_id,
            "",
            "https://api.example.com/event-route",
            "GET",
            "timeout_retry");

    vix::tests::Assert::is_true(
        result.success,
        "seed run should be created");

    return result.run.public_id;
  }

  void seed_event(
      const softadastra::cloud::app::AppState &state,
      const std::string &run_id,
      const std::string &event_type)
  {
    softadastra::cloud::modules::events::EventService service{state};

    const auto result =
        service.create_event(
            run_id,
            event_type,
            "warning",
            "Seeded event",
            R"({"seed":true})");

    vix::tests::Assert::is_true(
        result.success,
        "seed event should be created");
  }

  void register_event_routes_integration_tests()
  {
    vix::tests::TestSuite suite{"integration.events.EventRoutes"};

    suite.add(
        vix::tests::TestCase{
            "event routes register without throwing",
            []
            {
              auto state = make_state();

              vix::App app;

              softadastra::cloud::modules::events::EventRoutes::register_routes(
                  app,
                  state);

              vix::tests::Assert::is_true(
                  true,
                  "EventRoutes::register_routes should not throw");
            }});

    suite.add(
        vix::tests::TestCase{
            "event list endpoint returns 200",
            []
            {
              auto state = make_state();
              const std::string run_id = seed_run(state);

              seed_event(
                  state,
                  run_id,
                  "request_timeout");

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::events::EventController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "event list endpoint should return 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "event list endpoint should return a body");

              vix::tests::Assert::equal(
                  std::string{"application/json; charset=utf-8"},
                  response.native.header("Content-Type"),
                  "event list endpoint should return JSON");
            }});

    suite.add(
        vix::tests::TestCase{
            "event list endpoint contains seeded event",
            []
            {
              auto state = make_state();
              const std::string run_id = seed_run(state);

              seed_event(
                  state,
                  run_id,
                  "request_timeout");

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::events::EventController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::is_true(
                  response.native.body().find("request_timeout") !=
                      std::string::npos,
                  "response body should contain seeded event type");

              vix::tests::Assert::is_true(
                  response.native.body().find("Seeded event") !=
                      std::string::npos,
                  "response body should contain seeded event message");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_event_routes_integration_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
