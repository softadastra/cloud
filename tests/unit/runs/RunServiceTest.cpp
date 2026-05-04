/**
 *
 *  @file RunServiceTest.cpp
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
#include "modules/agents/AgentService.hpp"
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
            "Run Test Project",
            "Project used by run tests",
            "https://run-test.example.com");

    vix::tests::Assert::is_true(
        result.success,
        "test project should be created");

    return result.project.public_id;
  }

  [[nodiscard]] std::string create_agent(
      const softadastra::cloud::app::AppState &state,
      const std::string &project_id)
  {
    softadastra::cloud::modules::agents::AgentService service{state};

    const auto result =
        service.create_agent(
            project_id,
            "Run Test Agent");

    vix::tests::Assert::is_true(
        result.success,
        "test agent should be created");

    return result.agent.public_id;
  }

  void register_run_service_tests()
  {
    vix::tests::TestSuite suite{"unit.runs.RunService"};

    suite.add(
        vix::tests::TestCase{
            "create_run creates a run without agent",
            []
            {
              auto state = make_state();
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
                  "create_run should succeed");

              vix::tests::Assert::equal(
                  201,
                  result.status,
                  "create_run should return status 201");

              vix::tests::Assert::is_false(
                  result.run.public_id.empty(),
                  "created run should have a public id");

              vix::tests::Assert::equal(
                  std::string{"running"},
                  result.run.status);

              vix::tests::Assert::equal(
                  std::string{"GET"},
                  result.run.method);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run creates a run with agent",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);
              const std::string agent_id = create_agent(state, project_id);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      agent_id,
                      "https://api.example.com/orders",
                      "POST",
                      "unstable_network");

              vix::tests::Assert::is_true(
                  result.success,
                  "create_run with agent should succeed");

              vix::tests::Assert::equal(
                  std::string{"POST"},
                  result.run.method);

              vix::tests::Assert::equal(
                  agent_id,
                  result.run.agent_public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run normalizes lowercase method",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/items",
                      "patch",
                      "jitter");

              vix::tests::Assert::is_true(
                  result.success,
                  "lowercase HTTP method should be accepted");

              vix::tests::Assert::equal(
                  std::string{"PATCH"},
                  result.run.method);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run defaults empty method to GET",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/default-method",
                      "",
                      "default_method");

              vix::tests::Assert::is_true(
                  result.success,
                  "empty HTTP method should default to GET");

              vix::tests::Assert::equal(
                  std::string{"GET"},
                  result.run.method);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run rejects empty project id",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      "",
                      "",
                      "https://api.example.com",
                      "GET",
                      "timeout_retry");

              vix::tests::Assert::is_false(
                  result.success,
                  "empty project id should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);

              vix::tests::Assert::equal(
                  std::string{"validation_failed"},
                  result.error);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run rejects invalid target url",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      "",
                      "api.example.com",
                      "GET",
                      "timeout_retry");

              vix::tests::Assert::is_false(
                  result.success,
                  "invalid target URL should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run rejects invalid method",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com",
                      "CONNECT",
                      "timeout_retry");

              vix::tests::Assert::is_false(
                  result.success,
                  "invalid HTTP method should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run rejects invalid scenario",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com",
                      "GET",
                      "x");

              vix::tests::Assert::is_false(
                  result.success,
                  "too short scenario should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run returns 404 for missing project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      "prj_missing",
                      "",
                      "https://api.example.com",
                      "GET",
                      "timeout_retry");

              vix::tests::Assert::is_false(
                  result.success,
                  "missing project should fail");

              vix::tests::Assert::equal(
                  404,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_run rejects agent from missing id",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto result =
                  service.create_run(
                      project_id,
                      "agt_missing",
                      "https://api.example.com",
                      "GET",
                      "timeout_retry");

              vix::tests::Assert::is_false(
                  result.success,
                  "missing agent should fail");

              vix::tests::Assert::equal(
                  404,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "list_runs returns created runs",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              service.create_run(
                  project_id,
                  "",
                  "https://api.example.com/one",
                  "GET",
                  "timeout_retry");

              service.create_run(
                  project_id,
                  "",
                  "https://api.example.com/two",
                  "POST",
                  "network_jitter");

              const auto result =
                  service.list_runs(20, 0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_runs should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status);

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  result.total);

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(2),
                  result.runs.size());
            }});

    suite.add(
        vix::tests::TestCase{
            "list_runs_for_project returns project runs",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              service.create_run(
                  project_id,
                  "",
                  "https://api.example.com/one",
                  "GET",
                  "timeout_retry");

              const auto result =
                  service.list_runs_for_project(
                      project_id,
                      20,
                      0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_runs_for_project should succeed");

              vix::tests::Assert::equal(
                  static_cast<long long>(1),
                  result.total);
            }});

    suite.add(
        vix::tests::TestCase{
            "list_runs_for_agent returns agent runs",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);
              const std::string agent_id = create_agent(state, project_id);

              softadastra::cloud::modules::runs::RunService service{state};

              service.create_run(
                  project_id,
                  agent_id,
                  "https://api.example.com/agent",
                  "GET",
                  "timeout_retry");

              const auto result =
                  service.list_runs_for_agent(
                      agent_id,
                      20,
                      0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_runs_for_agent should succeed");

              vix::tests::Assert::equal(
                  static_cast<long long>(1),
                  result.total);
            }});

    suite.add(
        vix::tests::TestCase{
            "get_run returns existing run",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto created =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/get",
                      "GET",
                      "timeout_retry");

              const auto loaded =
                  service.get_run(created.run.public_id);

              vix::tests::Assert::is_true(
                  loaded.success,
                  "get_run should load existing run");

              vix::tests::Assert::equal(
                  created.run.public_id,
                  loaded.run.public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "update_status updates non-final run",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto created =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/status",
                      "GET",
                      "timeout_retry");

              const auto updated =
                  service.update_status(
                      created.run.public_id,
                      "created");

              vix::tests::Assert::is_true(
                  updated.success,
                  "update_status should succeed for non-final status");

              vix::tests::Assert::equal(
                  std::string{"created"},
                  updated.run.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "finish_run finishes run",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto created =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/finish",
                      "GET",
                      "timeout_retry");

              const auto finished =
                  service.finish_run(
                      created.run.public_id,
                      "finished");

              vix::tests::Assert::is_true(
                  finished.success,
                  "finish_run should succeed");

              vix::tests::Assert::equal(
                  std::string{"finished"},
                  finished.run.status);

              vix::tests::Assert::is_true(
                  finished.run.finished_at > 0,
                  "finished run should have finished_at timestamp");
            }});

    suite.add(
        vix::tests::TestCase{
            "finished run cannot be updated to non-final status",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto created =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/final",
                      "GET",
                      "timeout_retry");

              service.finish_run(
                  created.run.public_id,
                  "finished");

              const auto updated =
                  service.update_status(
                      created.run.public_id,
                      "running");

              vix::tests::Assert::is_false(
                  updated.success,
                  "finished run should not be updateable");
            }});

    suite.add(
        vix::tests::TestCase{
            "delete_run removes existing run",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::runs::RunService service{state};

              const auto created =
                  service.create_run(
                      project_id,
                      "",
                      "https://api.example.com/delete",
                      "GET",
                      "timeout_retry");

              const auto deleted =
                  service.delete_run(created.run.public_id);

              const auto loaded =
                  service.get_run(created.run.public_id);

              vix::tests::Assert::is_true(
                  deleted.success,
                  "delete_run should succeed");

              vix::tests::Assert::equal(
                  200,
                  deleted.status);

              vix::tests::Assert::is_false(
                  loaded.success,
                  "deleted run should not be found");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_run_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
