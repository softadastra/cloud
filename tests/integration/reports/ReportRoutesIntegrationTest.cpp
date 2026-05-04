/**
 *
 *  @file ReportRoutesIntegrationTest.cpp
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
#include "modules/projects/ProjectService.hpp"
#include "modules/reports/ReportController.hpp"
#include "modules/reports/ReportRoutes.hpp"
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
            "Report Route Project",
            "Project used by report route tests",
            "https://report-route.example.com");

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
            "https://api.example.com/report-route",
            "GET",
            "timeout_retry");

    vix::tests::Assert::is_true(
        result.success,
        "seed run should be created");

    return result.run.public_id;
  }

  void seed_report(
      const softadastra::cloud::app::AppState &state,
      const std::string &run_id)
  {
    softadastra::cloud::modules::reports::ReportService service{state};

    const auto result =
        service.submit_report(
            run_id,
            "Seeded report summary",
            R"({"score":87,"seed":true})");

    vix::tests::Assert::is_true(
        result.success,
        "seed report should be created");
  }

  void register_report_routes_integration_tests()
  {
    vix::tests::TestSuite suite{"integration.reports.ReportRoutes"};

    suite.add(
        vix::tests::TestCase{
            "report routes register without throwing",
            []
            {
              auto state = make_state();

              vix::App app;

              softadastra::cloud::modules::reports::ReportRoutes::register_routes(
                  app,
                  state);

              vix::tests::Assert::is_true(
                  true,
                  "ReportRoutes::register_routes should not throw");
            }});

    suite.add(
        vix::tests::TestCase{
            "report list endpoint returns 200",
            []
            {
              auto state = make_state();
              const std::string run_id = seed_run(state);

              seed_report(
                  state,
                  run_id);

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::reports::ReportController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "report list endpoint should return 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "report list endpoint should return a body");

              vix::tests::Assert::equal(
                  std::string{"application/json; charset=utf-8"},
                  response.native.header("Content-Type"),
                  "report list endpoint should return JSON");
            }});

    suite.add(
        vix::tests::TestCase{
            "report list endpoint contains seeded report",
            []
            {
              auto state = make_state();
              const std::string run_id = seed_run(state);

              seed_report(
                  state,
                  run_id);

              vix::Request request;
              TestResponse response;

              softadastra::cloud::modules::reports::ReportController::list(
                  request,
                  response.wrapped,
                  state);

              vix::tests::Assert::is_true(
                  response.native.body().find("Seeded report summary") !=
                      std::string::npos,
                  "response body should contain seeded report summary");

              vix::tests::Assert::is_true(
                  response.native.body().find(R"("score":87)") !=
                      std::string::npos,
                  "response body should contain seeded report payload");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_report_routes_integration_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
