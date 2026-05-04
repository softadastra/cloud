/**
 *
 *  @file EventServiceTest.cpp
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

#include <vix/db/db.hpp>
#include <vix/tests/tests.hpp>

#include "app/AppState.hpp"
#include "config/CloudConfig.hpp"
#include "database/Migrations.hpp"
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

  [[nodiscard]] std::string create_project(
      const softadastra::cloud::app::AppState &state)
  {
    softadastra::cloud::modules::projects::ProjectService service{state};

    const auto result =
        service.create_project(
            "Event Test Project",
            "Project used by event tests",
            "https://event-test.example.com");

    vix::tests::Assert::is_true(
        result.success,
        "test project should be created");

    return result.project.public_id;
  }

  [[nodiscard]] std::string create_run(
      const softadastra::cloud::app::AppState &state)
  {
    const std::string project_id = create_project(state);

    softadastra::cloud::modules::runs::RunService service{state};

    const auto result =
        service.create_run(
            project_id,
            "",
            "https://api.example.com/health",
            "GET",
            "timeout_retry");

    vix::tests::Assert::is_true(
        result.success,
        "test run should be created");

    return result.run.public_id;
  }

  void register_event_service_tests()
  {
    vix::tests::TestSuite suite{"unit.events.EventService"};

    suite.add(
        vix::tests::TestCase{
            "create_event creates an event",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      run_id,
                      "request_timeout",
                      "warning",
                      "Request timed out after 5000ms",
                      R"({"duration_ms":5000})");

              vix::tests::Assert::is_true(
                  result.success,
                  "create_event should succeed");

              vix::tests::Assert::equal(
                  201,
                  result.status,
                  "create_event should return status 201");

              vix::tests::Assert::is_false(
                  result.event.public_id.empty(),
                  "created event should have a public id");

              vix::tests::Assert::equal(
                  std::string{"request_timeout"},
                  result.event.event_type);

              vix::tests::Assert::equal(
                  std::string{"warning"},
                  result.event.severity);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event normalizes warn severity",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      run_id,
                      "request_timeout",
                      "warn",
                      "Request timeout warning",
                      "{}");

              vix::tests::Assert::is_true(
                  result.success,
                  "warn severity should be accepted");

              vix::tests::Assert::equal(
                  std::string{"warning"},
                  result.event.severity,
                  "warn should be normalized to warning");
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event defaults empty severity to info",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      run_id,
                      "agent_started",
                      "",
                      "Agent started",
                      "{}");

              vix::tests::Assert::is_true(
                  result.success,
                  "empty severity should default to info");

              vix::tests::Assert::equal(
                  std::string{"info"},
                  result.event.severity);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event rejects empty run id",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      "",
                      "request_timeout",
                      "warning",
                      "Missing run",
                      "{}");

              vix::tests::Assert::is_false(
                  result.success,
                  "empty run id should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);

              vix::tests::Assert::equal(
                  std::string{"validation_failed"},
                  result.error);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event rejects invalid event type",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      run_id,
                      "x",
                      "info",
                      "Invalid event type",
                      "{}");

              vix::tests::Assert::is_false(
                  result.success,
                  "too short event type should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event rejects invalid severity",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      run_id,
                      "request_timeout",
                      "fatal",
                      "Invalid severity",
                      "{}");

              vix::tests::Assert::is_false(
                  result.success,
                  "invalid severity should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event rejects too long message",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const std::string message(1001, 'x');

              const auto result =
                  service.create_event(
                      run_id,
                      "request_timeout",
                      "warning",
                      message,
                      "{}");

              vix::tests::Assert::is_false(
                  result.success,
                  "too long message should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event rejects too large payload",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const std::string payload((1024 * 1024) + 1, 'x');

              const auto result =
                  service.create_event(
                      run_id,
                      "request_timeout",
                      "warning",
                      "Payload too large",
                      payload);

              vix::tests::Assert::is_false(
                  result.success,
                  "too large payload should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_event returns 404 for missing run",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::events::EventService service{state};

              const auto result =
                  service.create_event(
                      "run_missing",
                      "request_timeout",
                      "warning",
                      "Missing run",
                      "{}");

              vix::tests::Assert::is_false(
                  result.success,
                  "missing run should fail");

              vix::tests::Assert::equal(
                  404,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "list_events returns created events",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              service.create_event(
                  run_id,
                  "agent_started",
                  "info",
                  "Agent started",
                  "{}");

              service.create_event(
                  run_id,
                  "request_timeout",
                  "warning",
                  "Request timeout",
                  "{}");

              const auto result =
                  service.list_events(50, 0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_events should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status);

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  result.total);

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(2),
                  result.events.size());
            }});

    suite.add(
        vix::tests::TestCase{
            "list_events_for_run returns run events",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              service.create_event(
                  run_id,
                  "agent_started",
                  "info",
                  "Agent started",
                  "{}");

              const auto result =
                  service.list_events_for_run(
                      run_id,
                      50,
                      0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_events_for_run should succeed");

              vix::tests::Assert::equal(
                  static_cast<long long>(1),
                  result.total);
            }});

    suite.add(
        vix::tests::TestCase{
            "get_event returns existing event",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              const auto created =
                  service.create_event(
                      run_id,
                      "request_timeout",
                      "warning",
                      "Request timeout",
                      "{}");

              const auto loaded =
                  service.get_event(created.event.public_id);

              vix::tests::Assert::is_true(
                  loaded.success,
                  "get_event should load existing event");

              vix::tests::Assert::equal(
                  created.event.public_id,
                  loaded.event.public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "count_by_severity_for_run counts matching events",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              service.create_event(
                  run_id,
                  "request_timeout",
                  "warning",
                  "Request timeout",
                  "{}");

              service.create_event(
                  run_id,
                  "recovery_failed",
                  "error",
                  "Recovery failed",
                  "{}");

              const auto warning_count =
                  service.count_by_severity_for_run(
                      run_id,
                      "warning");

              const auto error_count =
                  service.count_by_severity_for_run(
                      run_id,
                      "error");

              vix::tests::Assert::equal(
                  static_cast<long long>(1),
                  warning_count);

              vix::tests::Assert::equal(
                  static_cast<long long>(1),
                  error_count);
            }});

    suite.add(
        vix::tests::TestCase{
            "count_by_type_for_run counts matching events",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::events::EventService service{state};

              service.create_event(
                  run_id,
                  "request_timeout",
                  "warning",
                  "First timeout",
                  "{}");

              service.create_event(
                  run_id,
                  "request_timeout",
                  "warning",
                  "Second timeout",
                  "{}");

              const auto count =
                  service.count_by_type_for_run(
                      run_id,
                      "request_timeout");

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  count);
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_event_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
