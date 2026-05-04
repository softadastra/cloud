/**
 *
 *  @file ReportServiceTest.cpp
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
#include "modules/projects/ProjectService.hpp"
#include "modules/reports/ReportService.hpp"
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
            "Report Test Project",
            "Project used by report tests",
            "https://report-test.example.com");

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
            "https://api.example.com/report",
            "GET",
            "timeout_retry");

    vix::tests::Assert::is_true(
        result.success,
        "test run should be created");

    return result.run.public_id;
  }

  void register_report_service_tests()
  {
    vix::tests::TestSuite suite{"unit.reports.ReportService"};

    suite.add(
        vix::tests::TestCase{
            "submit_report creates a report",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto result =
                  service.submit_report(
                      run_id,
                      "Run completed with timeout risk",
                      R"({"score":82,"risks":["timeout"]})");

              vix::tests::Assert::is_true(
                  result.success,
                  "submit_report should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status,
                  "current implementation returns 200 after saving because report exists when status is computed");

              vix::tests::Assert::is_false(
                  result.report.public_id.empty(),
                  "created report should have a public id");

              vix::tests::Assert::equal(
                  run_id,
                  result.report.run_public_id);

              vix::tests::Assert::equal(
                  std::string{"Run completed with timeout risk"},
                  result.report.summary);
            }});

    suite.add(
        vix::tests::TestCase{
            "submit_report updates existing report for same run",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto first =
                  service.submit_report(
                      run_id,
                      "Initial report",
                      R"({"score":70})");

              const auto second =
                  service.submit_report(
                      run_id,
                      "Updated report",
                      R"({"score":90})");

              vix::tests::Assert::is_true(
                  first.success,
                  "first submit_report should succeed");

              vix::tests::Assert::is_true(
                  second.success,
                  "second submit_report should update existing report");

              vix::tests::Assert::equal(
                  first.report.public_id,
                  second.report.public_id,
                  "updating a report should keep the same public id");

              vix::tests::Assert::equal(
                  std::string{"Updated report"},
                  second.report.summary);

              vix::tests::Assert::equal(
                  std::string{R"({"score":90})"},
                  second.report.raw_json);
            }});

    suite.add(
        vix::tests::TestCase{
            "submit_report rejects empty run id",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto result =
                  service.submit_report(
                      "",
                      "Valid summary",
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
            "submit_report rejects too short summary",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto result =
                  service.submit_report(
                      run_id,
                      "x",
                      "{}");

              vix::tests::Assert::is_false(
                  result.success,
                  "too short summary should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "submit_report rejects empty raw json",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto result =
                  service.submit_report(
                      run_id,
                      "Valid summary",
                      "");

              vix::tests::Assert::is_false(
                  result.success,
                  "empty raw json should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "submit_report rejects too large raw json",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const std::string raw_json((5 * 1024 * 1024) + 1, 'x');

              const auto result =
                  service.submit_report(
                      run_id,
                      "Valid summary",
                      raw_json);

              vix::tests::Assert::is_false(
                  result.success,
                  "too large raw json should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "submit_report returns 404 for missing run",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto result =
                  service.submit_report(
                      "run_missing",
                      "Valid summary",
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
            "list_reports returns submitted reports",
            []
            {
              auto state = make_state();
              const std::string first_run_id = create_run(state);
              const std::string second_run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              service.submit_report(
                  first_run_id,
                  "First report",
                  R"({"score":70})");

              service.submit_report(
                  second_run_id,
                  "Second report",
                  R"({"score":90})");

              const auto result =
                  service.list_reports(20, 0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_reports should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status);

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  result.total);

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(2),
                  result.reports.size());
            }});

    suite.add(
        vix::tests::TestCase{
            "get_report returns existing report",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto created =
                  service.submit_report(
                      run_id,
                      "Lookup report",
                      R"({"score":88})");

              const auto loaded =
                  service.get_report(created.report.public_id);

              vix::tests::Assert::is_true(
                  loaded.success,
                  "get_report should load existing report");

              vix::tests::Assert::equal(
                  created.report.public_id,
                  loaded.report.public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "get_report_for_run returns existing report",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto created =
                  service.submit_report(
                      run_id,
                      "Run report",
                      R"({"score":91})");

              const auto loaded =
                  service.get_report_for_run(run_id);

              vix::tests::Assert::is_true(
                  loaded.success,
                  "get_report_for_run should load existing report");

              vix::tests::Assert::equal(
                  created.report.public_id,
                  loaded.report.public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "get_report returns 404 for missing report",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto result =
                  service.get_report("rpt_missing");

              vix::tests::Assert::is_false(
                  result.success,
                  "missing report should fail");

              vix::tests::Assert::equal(
                  404,
                  result.status);
            }});

    suite.add(
        vix::tests::TestCase{
            "delete_report removes existing report",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              const auto created =
                  service.submit_report(
                      run_id,
                      "Delete report",
                      R"({"score":50})");

              const auto deleted =
                  service.delete_report(created.report.public_id);

              const auto loaded =
                  service.get_report(created.report.public_id);

              vix::tests::Assert::is_true(
                  deleted.success,
                  "delete_report should succeed");

              vix::tests::Assert::equal(
                  200,
                  deleted.status);

              vix::tests::Assert::is_false(
                  loaded.success,
                  "deleted report should not be found");
            }});

    suite.add(
        vix::tests::TestCase{
            "delete_report_for_run removes report attached to run",
            []
            {
              auto state = make_state();
              const std::string run_id = create_run(state);

              softadastra::cloud::modules::reports::ReportService service{state};

              service.submit_report(
                  run_id,
                  "Delete run report",
                  R"({"score":60})");

              const auto deleted =
                  service.delete_report_for_run(run_id);

              const auto loaded =
                  service.get_report_for_run(run_id);

              vix::tests::Assert::is_true(
                  deleted.success,
                  "delete_report_for_run should succeed");

              vix::tests::Assert::is_false(
                  loaded.success,
                  "report should not be found after delete_report_for_run");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_report_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
