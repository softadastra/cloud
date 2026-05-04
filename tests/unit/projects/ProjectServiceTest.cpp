/**
 *
 *  @file ProjectServiceTest.cpp
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

  void register_project_service_tests()
  {
    vix::tests::TestSuite suite{"unit.projects.ProjectService"};

    suite.add(
        vix::tests::TestCase{
            "create_project creates a project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto result =
                  service.create_project(
                      "PulseGrid API",
                      "Production monitoring API",
                      "https://pulsegrid.softadastra.com");

              vix::tests::Assert::is_true(
                  result.success,
                  "create_project should succeed");

              vix::tests::Assert::equal(
                  201,
                  result.status,
                  "create_project should return status 201");

              vix::tests::Assert::is_false(
                  result.project.public_id.empty(),
                  "created project should have a public id");

              vix::tests::Assert::equal(
                  std::string{"PulseGrid API"},
                  result.project.name);

              vix::tests::Assert::equal(
                  std::string{"pulsegrid-api"},
                  result.project.slug);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_project rejects empty name",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto result =
                  service.create_project(
                      "",
                      "Invalid project",
                      "https://api.example.com");

              vix::tests::Assert::is_false(
                  result.success,
                  "empty project name should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status,
                  "empty project name should return status 400");

              vix::tests::Assert::equal(
                  std::string{"validation_failed"},
                  result.error);
            }});

    suite.add(
        vix::tests::TestCase{
            "create_project accepts empty base url",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto result =
                  service.create_project(
                      "No Base URL",
                      "Project without base URL",
                      "");

              vix::tests::Assert::is_true(
                  result.success,
                  "empty base URL should be allowed");
            }});

    suite.add(
        vix::tests::TestCase{
            "create_project rejects invalid base url",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto result =
                  service.create_project(
                      "Invalid Base URL",
                      "Invalid project",
                      "api.example.com");

              vix::tests::Assert::is_false(
                  result.success,
                  "invalid base URL should fail");

              vix::tests::Assert::equal(
                  400,
                  result.status,
                  "invalid base URL should return status 400");
            }});

    suite.add(
        vix::tests::TestCase{
            "duplicate project names generate unique slugs",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto first =
                  service.create_project(
                      "Duplicate Project",
                      "First",
                      "https://first.example.com");

              const auto second =
                  service.create_project(
                      "Duplicate Project",
                      "Second",
                      "https://second.example.com");

              vix::tests::Assert::is_true(
                  first.success,
                  "first project should be created");

              vix::tests::Assert::is_true(
                  second.success,
                  "second project with same name should also be created");

              vix::tests::Assert::equal(
                  std::string{"duplicate-project"},
                  first.project.slug);

              vix::tests::Assert::equal(
                  std::string{"duplicate-project-2"},
                  second.project.slug);
            }});

    suite.add(
        vix::tests::TestCase{
            "list_projects returns created projects",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              service.create_project(
                  "Project One",
                  "First project",
                  "https://one.example.com");

              service.create_project(
                  "Project Two",
                  "Second project",
                  "https://two.example.com");

              const auto result =
                  service.list_projects(20, 0);

              vix::tests::Assert::is_true(
                  result.success,
                  "list_projects should succeed");

              vix::tests::Assert::equal(
                  200,
                  result.status,
                  "list_projects should return status 200");

              vix::tests::Assert::equal(
                  static_cast<long long>(2),
                  result.total,
                  "list_projects should return total count");

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(2),
                  result.projects.size(),
                  "list_projects should return two projects");
            }});

    suite.add(
        vix::tests::TestCase{
            "get_project returns existing project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto created =
                  service.create_project(
                      "Lookup Project",
                      "Project to load",
                      "https://lookup.example.com");

              const auto loaded =
                  service.get_project(created.project.public_id);

              vix::tests::Assert::is_true(
                  loaded.success,
                  "get_project should load existing project");

              vix::tests::Assert::equal(
                  created.project.public_id,
                  loaded.project.public_id);

              vix::tests::Assert::equal(
                  std::string{"Lookup Project"},
                  loaded.project.name);
            }});

    suite.add(
        vix::tests::TestCase{
            "get_project returns 404 for missing project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto result =
                  service.get_project("prj_missing");

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
            "update_project updates existing project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto created =
                  service.create_project(
                      "Update Project",
                      "Before update",
                      "https://before.example.com");

              const auto updated =
                  service.update_project(
                      created.project.public_id,
                      "Updated Project",
                      "After update",
                      "https://after.example.com");

              vix::tests::Assert::is_true(
                  updated.success,
                  "update_project should succeed");

              vix::tests::Assert::equal(
                  std::string{"Updated Project"},
                  updated.project.name);

              vix::tests::Assert::equal(
                  std::string{"After update"},
                  updated.project.description);

              vix::tests::Assert::equal(
                  std::string{"https://after.example.com"},
                  updated.project.base_url);
            }});

    suite.add(
        vix::tests::TestCase{
            "delete_project removes existing project",
            []
            {
              auto state = make_state();

              softadastra::cloud::modules::projects::ProjectService service{
                  state};

              const auto created =
                  service.create_project(
                      "Delete Project",
                      "Project to delete",
                      "https://delete.example.com");

              const auto deleted =
                  service.delete_project(created.project.public_id);

              const auto loaded =
                  service.get_project(created.project.public_id);

              vix::tests::Assert::is_true(
                  deleted.success,
                  "delete_project should succeed");

              vix::tests::Assert::equal(
                  200,
                  deleted.status,
                  "delete_project should return status 200");

              vix::tests::Assert::is_false(
                  loaded.success,
                  "deleted project should not be found");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_project_service_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
