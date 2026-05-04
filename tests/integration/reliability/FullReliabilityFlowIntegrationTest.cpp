/**
 *
 *  @file FullReliabilityFlowIntegrationTest.cpp
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
#include "modules/events/EventService.hpp"
#include "modules/projects/ProjectService.hpp"
#include "modules/reports/ReportService.hpp"
#include "modules/runs/RunService.hpp"
#include "modules/scoring/ScoringService.hpp"

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

  void register_full_reliability_flow_tests()
  {
    vix::tests::TestSuite suite{"integration.reliability.FullFlow"};

    suite.add(
        vix::tests::TestCase{
            "full reliability flow creates project agent run events score and report",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService project_service{
                  state};

              softadastra::cloud::modules::agents::AgentService agent_service{
                  state};

              softadastra::cloud::modules::runs::RunService run_service{
                  state};

              softadastra::cloud::modules::events::EventService event_service{
                  state};

              softadastra::cloud::modules::scoring::ScoringService scoring_service{
                  state};

              softadastra::cloud::modules::reports::ReportService report_service{
                  state};

              const auto project =
                  project_service.create_project(
                      "Full Reliability Project",
                      "Integration test project",
                      "https://full-flow.example.com");

              vix::tests::Assert::is_true(
                  project.success,
                  "project should be created");

              const auto agent =
                  agent_service.create_agent(
                      project.project.public_id,
                      "Full Flow Agent");

              vix::tests::Assert::is_true(
                  agent.success,
                  "agent should be created");

              vix::tests::Assert::is_false(
                  agent.api_key.empty(),
                  "agent should receive a one-time API key");

              const auto authenticated =
                  agent_service.authenticate_api_key(agent.api_key);

              vix::tests::Assert::is_true(
                  authenticated.success,
                  "created agent API key should authenticate");

              vix::tests::Assert::equal(
                  agent.agent.public_id,
                  authenticated.agent.public_id);

              const auto run =
                  run_service.create_run(
                      project.project.public_id,
                      agent.agent.public_id,
                      "https://api.example.com/orders",
                      "POST",
                      "unstable_network");

              vix::tests::Assert::is_true(
                  run.success,
                  "run should be created");

              vix::tests::Assert::equal(
                  std::string{"running"},
                  run.run.status);

              const auto started_event =
                  event_service.create_event(
                      run.run.public_id,
                      "agent_started",
                      "info",
                      "Agent started reliability test",
                      R"({"step":"started"})");

              const auto timeout_event =
                  event_service.create_event(
                      run.run.public_id,
                      "request_timeout",
                      "warning",
                      "Request timed out and retry was triggered",
                      R"({"duration_ms":5000})");

              const auto recovery_event =
                  event_service.create_event(
                      run.run.public_id,
                      "recovery_succeeded",
                      "info",
                      "Request recovered after retry",
                      R"({"retry_count":1})");

              vix::tests::Assert::is_true(
                  started_event.success,
                  "started event should be created");

              vix::tests::Assert::is_true(
                  timeout_event.success,
                  "timeout event should be created");

              vix::tests::Assert::is_true(
                  recovery_event.success,
                  "recovery event should be created");

              const auto events =
                  event_service.list_events_for_run(
                      run.run.public_id,
                      50,
                      0);

              vix::tests::Assert::is_true(
                  events.success,
                  "events should be listed for run");

              vix::tests::Assert::equal(
                  static_cast<long long>(3),
                  events.total,
                  "run should contain three events");

              const auto finished =
                  run_service.finish_run(
                      run.run.public_id,
                      "finished");

              vix::tests::Assert::is_true(
                  finished.success,
                  "run should finish successfully");

              vix::tests::Assert::equal(
                  std::string{"finished"},
                  finished.run.status);

              const auto score =
                  scoring_service.compute_for_run(
                      run.run.public_id);

              vix::tests::Assert::is_true(
                  score.success,
                  "score should be computed");

              vix::tests::Assert::equal(
                  87,
                  score.score.score,
                  "timeout event with warning should produce score 87");

              vix::tests::Assert::equal(
                  std::string{"B"},
                  score.score.grade);

              vix::tests::Assert::is_true(
                  score.score.timeout_risk,
                  "timeout risk should be detected");

              const auto report =
                  report_service.submit_report(
                      run.run.public_id,
                      "Reliability test completed with timeout risk",
                      R"({"score":87,"grade":"B","risks":["timeout"]})");

              vix::tests::Assert::is_true(
                  report.success,
                  "report should be submitted");

              vix::tests::Assert::equal(
                  run.run.public_id,
                  report.report.run_public_id);

              const auto loaded_report =
                  report_service.get_report_for_run(
                      run.run.public_id);

              vix::tests::Assert::is_true(
                  loaded_report.success,
                  "report should be loaded by run id");

              vix::tests::Assert::equal(
                  report.report.public_id,
                  loaded_report.report.public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "full reliability flow detects dangerous data loss scenario",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService project_service{
                  state};

              softadastra::cloud::modules::agents::AgentService agent_service{
                  state};

              softadastra::cloud::modules::runs::RunService run_service{
                  state};

              softadastra::cloud::modules::events::EventService event_service{
                  state};

              softadastra::cloud::modules::scoring::ScoringService scoring_service{
                  state};

              const auto project =
                  project_service.create_project(
                      "Danger Flow Project",
                      "Danger integration test project",
                      "https://danger-flow.example.com");

              vix::tests::Assert::is_true(
                  project.success,
                  "project should be created");

              const auto agent =
                  agent_service.create_agent(
                      project.project.public_id,
                      "Danger Flow Agent");

              vix::tests::Assert::is_true(
                  agent.success,
                  "agent should be created");

              const auto run =
                  run_service.create_run(
                      project.project.public_id,
                      agent.agent.public_id,
                      "https://api.example.com/payments",
                      "POST",
                      "network_partition");

              vix::tests::Assert::is_true(
                  run.success,
                  "run should be created");

              event_service.create_event(
                  run.run.public_id,
                  "data_loss_risk_detected",
                  "critical",
                  "Local write was not confirmed after network failure",
                  R"({"risk":"data_loss"})");

              event_service.create_event(
                  run.run.public_id,
                  "duplicate_detected",
                  "error",
                  "Duplicate operation detected after retry",
                  R"({"risk":"duplicate"})");

              event_service.create_event(
                  run.run.public_id,
                  "recovery_failed",
                  "error",
                  "Application failed to recover after restart",
                  R"({"risk":"recovery_failed"})");

              const auto finished =
                  run_service.finish_run(
                      run.run.public_id,
                      "failed");

              vix::tests::Assert::is_true(
                  finished.success,
                  "danger flow run should finish as failed");

              const auto score =
                  scoring_service.compute_for_run(
                      run.run.public_id);

              vix::tests::Assert::is_true(
                  score.success,
                  "score should be computed");

              vix::tests::Assert::equal(
                  0,
                  score.score.score,
                  "dangerous scenario should clamp score to zero");

              vix::tests::Assert::equal(
                  std::string{"F"},
                  score.score.grade);

              vix::tests::Assert::is_true(
                  score.score.data_loss_risk,
                  "data loss risk should be detected");

              vix::tests::Assert::is_true(
                  score.score.duplicate_risk,
                  "duplicate risk should be detected");

              vix::tests::Assert::is_true(
                  score.score.recovery_risk,
                  "recovery risk should be detected");

              vix::tests::Assert::is_true(
                  score.score.is_dangerous(),
                  "score should be dangerous");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_full_reliability_flow_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
