/**
 *
 *  @file ScoringServiceTest.cpp
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

  [[nodiscard]] std::string create_project(
      const softadastra::cloud::app::AppState &state)
  {
    softadastra::cloud::modules::projects::ProjectService service{state};

    const auto result =
        service.create_project(
            "Scoring Test Project",
            "Project used by scoring tests",
            "https://scoring-test.example.com");

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
            "https://api.example.com/scoring",
            "GET",
            "timeout_retry");

    vix::tests::Assert::is_true(
        result.success,
        "test run should be created");

    return result.run.public_id;
  }

  void add_event(
      const softadastra::cloud::app::AppState &state,
      const std::string &run_id,
      const std::string &event_type,
      const std::string &severity)
  {
    softadastra::cloud::modules::events::EventService service{state};

    const auto result =
        service.create_event(
            run_id,
            event_type,
            severity,
            "Scoring event",
            "{}");

    vix::tests::Assert::is_true(
        result.success,
        "test event should be created");
  }

  void register_scoring_service_tests()
  {
    vix::tests::TestSuite suite{"unit.scoring.ScoringService"};

    suite.add(
        vix::tests::TestCase{
            "compute_for_run returns perfect score when no risky events exist",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run(run_id);

              vix::tests::Assert::is_true(
                  result.success,
                  "compute_for_run should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status);

              vix::tests::Assert::equal(
                  100,
                  result.score.score);

              vix::tests::Assert::equal(
                  std::string{"A"},
                  result.score.grade);

              vix::tests::Assert::is_false(
                  result.score.has_any_risk(),
                  "score should not contain risk flags");

              vix::tests::Assert::is_true(
                  result.score.is_excellent(),
                  "score 100 should be excellent");
            }});

    suite.add(
        vix::tests::TestCase{
            "compute_for_run detects timeout risk",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              add_event(
                  state,
                  run_id,
                  "request_timeout",
                  "warning");

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run(run_id);

              vix::tests::Assert::is_true(
                  result.success,
                  "compute_for_run should succeed");

              vix::tests::Assert::equal(
                  87,
                  result.score.score,
                  "timeout risk subtracts 10 and warning subtracts 3");

              vix::tests::Assert::equal(
                  std::string{"B"},
                  result.score.grade);

              vix::tests::Assert::is_true(
                  result.score.timeout_risk,
                  "timeout risk should be true");

              vix::tests::Assert::is_true(
                  result.score.has_any_risk(),
                  "score should contain at least one risk flag");
            }});

    suite.add(
        vix::tests::TestCase{
            "compute_for_run detects data loss risk",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              add_event(
                  state,
                  run_id,
                  "data_loss_risk_detected",
                  "critical");

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run(run_id);

              vix::tests::Assert::is_true(
                  result.success,
                  "compute_for_run should succeed");

              vix::tests::Assert::equal(
                  50,
                  result.score.score,
                  "data loss risk subtracts 35 and critical subtracts 15");

              vix::tests::Assert::equal(
                  std::string{"D"},
                  result.score.grade);

              vix::tests::Assert::is_true(
                  result.score.data_loss_risk,
                  "data_loss_risk should be true");
            }});

    suite.add(
        vix::tests::TestCase{
            "compute_for_run detects duplicate risk",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              add_event(
                  state,
                  run_id,
                  "duplicate_detected",
                  "error");

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run(run_id);

              vix::tests::Assert::is_true(
                  result.success,
                  "compute_for_run should succeed");

              vix::tests::Assert::equal(
                  72,
                  result.score.score,
                  "duplicate risk subtracts 20 and error subtracts 8");

              vix::tests::Assert::equal(
                  std::string{"C"},
                  result.score.grade);

              vix::tests::Assert::is_true(
                  result.score.duplicate_risk,
                  "duplicate_risk should be true");
            }});

    suite.add(
        vix::tests::TestCase{
            "compute_for_run detects recovery risk",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              add_event(
                  state,
                  run_id,
                  "recovery_failed",
                  "error");

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run(run_id);

              vix::tests::Assert::is_true(
                  result.success,
                  "compute_for_run should succeed");

              vix::tests::Assert::equal(
                  67,
                  result.score.score,
                  "recovery risk subtracts 25 and error subtracts 8");

              vix::tests::Assert::equal(
                  std::string{"C"},
                  result.score.grade);

              vix::tests::Assert::is_true(
                  result.score.recovery_risk,
                  "recovery_risk should be true");
            }});

    suite.add(
        vix::tests::TestCase{
            "compute_for_run clamps score to zero",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              add_event(state, run_id, "data_loss_risk_detected", "critical");
              add_event(state, run_id, "duplicate_detected", "critical");
              add_event(state, run_id, "request_timeout", "critical");
              add_event(state, run_id, "recovery_failed", "critical");
              add_event(state, run_id, "recovery_failed", "error");
              add_event(state, run_id, "request_timeout", "warning");

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run(run_id);

              vix::tests::Assert::is_true(
                  result.success,
                  "compute_for_run should succeed");

              vix::tests::Assert::equal(
                  0,
                  result.score.score,
                  "score should be clamped to zero");

              vix::tests::Assert::equal(
                  std::string{"F"},
                  result.score.grade);

              vix::tests::Assert::is_true(
                  result.score.is_dangerous(),
                  "score 0 should be dangerous");
            }});

    suite.add(
        vix::tests::TestCase{
            "compute_for_run rejects empty run id",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run("");

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
            "compute_for_run returns 404 for missing run",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::scoring::ScoringService service{state};

              const auto result =
                  service.compute_for_run("run_missing");

              vix::tests::Assert::is_false(
                  result.success,
                  "missing run should fail");

              vix::tests::Assert::equal(
                  404,
                  result.status);

              vix::tests::Assert::equal(
                  std::string{"run_not_found"},
                  result.error);
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_scoring_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
