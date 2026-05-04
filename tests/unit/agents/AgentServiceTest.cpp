/**
 *
 *  @file AgentServiceTest.cpp
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
            "Agent Test Project",
            "Project used by agent tests",
            "https://agent-test.example.com");

    vix::tests::Assert::is_true(
        result.success,
        "test project should be created");

    return result.project.public_id;
  }

  void register_agent_service_tests()
  {
    vix::tests::TestSuite suite{"unit.agents.AgentService"};

    suite.add(
        vix::tests::TestCase{
            "create_agent creates an agent with one-time API key",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto result =
                  service.create_agent(
                      project_id,
                      "Local Agent");

              vix::tests::Assert::is_true(
                  result.success,
                  "create_agent should succeed");

              vix::tests::Assert::equal(
                  201,
                  result.status,
                  "create_agent should return status 201");

              vix::tests::Assert::is_false(
                  result.agent.public_id.empty(),
                  "created agent should have a public id");

              vix::tests::Assert::equal(
                  std::string{"Local Agent"},
                  result.agent.name);

              vix::tests::Assert::is_false(
                  result.api_key.empty(),
                  "created agent should return one-time plain API key");

              vix::tests::Assert::is_true(
                  result.api_key.rfind("sa_agent_key_", 0) == 0,
                  "agent API key should use expected prefix");
            }});

    suite.add(
        vix::tests::TestCase{
            "create_agent rejects empty project id",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto result =
                  service.create_agent(
                      "",
                      "Local Agent");

              vix::tests::Assert::is_false(
                  result.success,
                  "empty project id should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status,
                  "empty project id should return status 400");

              vix::tests::Assert::equal(
                  std::string{"validation_failed"},
                  result.error);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_agent rejects empty name",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto result =
                  service.create_agent(
                      project_id,
                      "");

              vix::tests::Assert::is_false(
                  result.success,
                  "empty agent name should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status,
                  "empty agent name should return status 400");
            }});

    suite.add(
        vix::tests::TestCase{
            "create_agent returns 404 for missing project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto result =
                  service.create_agent(
                      "prj_missing",
                      "Missing Project Agent");

              vix::tests::Assert::is_false(
                  result.success,
                  "missing project should fail");

              vix::tests::Assert::equal(
                  404,
                  result.status,
                  "missing project should return status 404");
            }});

    suite.add(
        vix::tests::TestCase{
            "list_agents returns created agents",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              service.create_agent(
                  project_id,
                  "Agent One");

              service.create_agent(
                  project_id,
                  "Agent Two");

              const auto result =
                  service.list_agents(20, 0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_agents should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status,
                  "list_agents should return status 200");

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  result.total,
                  "list_agents should count two agents");

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(2),
                  result.agents.size(),
                  "list_agents should return two agents");
            }});

    suite.add(
        vix::tests::TestCase{
            "list_agents_for_project returns project agents",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              service.create_agent(
                  project_id,
                  "Project Agent One");

              service.create_agent(
                  project_id,
                  "Project Agent Two");

              const auto result =
                  service.list_agents_for_project(
                      project_id,
                      20,
                      0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_agents_for_project should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status,
                  "list_agents_for_project should return status 200");

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  result.total,
                  "project should have two agents");
            }});

    suite.add(
        vix::tests::TestCase{
            "get_agent returns existing agent",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto created =
                  service.create_agent(
                      project_id,
                      "Lookup Agent");

              const auto loaded =
                  service.get_agent(created.agent.public_id);

              vix::tests::Assert::is_true(
                  loaded.success,
                  "get_agent should load existing agent");

              vix::tests::Assert::equal(
                  created.agent.public_id,
                  loaded.agent.public_id);

              vix::tests::Assert::equal(
                  std::string{"Lookup Agent"},
                  loaded.agent.name);
            }});

    suite.add(
        vix::tests::TestCase{
            "authenticate_api_key accepts created API key",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto created =
                  service.create_agent(
                      project_id,
                      "Auth Agent");

              const auto authenticated =
                  service.authenticate_api_key(created.api_key);

              vix::tests::Assert::is_true(
                  authenticated.success,
                  "created API key should authenticate");

              vix::tests::Assert::equal(
                  200,
                  authenticated.status,
                  "authenticate_api_key should return status 200");

              vix::tests::Assert::equal(
                  created.agent.public_id,
                  authenticated.agent.public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "authenticate_api_key rejects invalid API key",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto result =
                  service.authenticate_api_key("sa_agent_invalid_key");

              vix::tests::Assert::is_false(
                  result.success,
                  "invalid API key should fail");

              vix::tests::Assert::equal(
                  401,
                  result.status,
                  "invalid API key should return status 401");
            }});

    suite.add(
        vix::tests::TestCase{
            "rotate_api_key returns a new API key",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto created =
                  service.create_agent(
                      project_id,
                      "Rotating Agent");

              const auto rotated =
                  service.rotate_api_key(created.agent.public_id);

              vix::tests::Assert::is_true(
                  rotated.success,
                  "rotate_api_key should succeed");

              vix::tests::Assert::equal(
                  200,
                  rotated.status,
                  "rotate_api_key should return status 200");

              vix::tests::Assert::is_false(
                  rotated.api_key.empty(),
                  "rotated result should include new API key");

              vix::tests::Assert::not_equal(
                  created.api_key,
                  rotated.api_key,
                  "rotated API key should differ from original API key");
            }});

    suite.add(
        vix::tests::TestCase{
            "delete_agent removes existing agent",
            []
            {
              auto state = make_state();
              const std::string project_id = create_project(state);

              softadastra::cloud::modules::agents::AgentService service{state};

              const auto created =
                  service.create_agent(
                      project_id,
                      "Delete Agent");

              const auto deleted =
                  service.delete_agent(created.agent.public_id);

              const auto loaded =
                  service.get_agent(created.agent.public_id);

              vix::tests::Assert::is_true(
                  deleted.success,
                  "delete_agent should succeed");

              vix::tests::Assert::equal(
                  200,
                  deleted.status,
                  "delete_agent should return status 200");

              vix::tests::Assert::is_false(
                  loaded.success,
                  "deleted agent should not be found");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_agent_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
