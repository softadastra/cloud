/**
 *
 *  @file test_projects.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Softadastra Cloud
 */
#include <projects/ProjectsModule.hpp>
#include <projects/services/ProjectService.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("projects module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::projects::ProjectsModule::name()),
                              std::string("projects")); }));

  registry.add(TestCase("project service creates a valid project", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "Softadastra Cloud";
                          request.slug = "softadastra-cloud";
                          request.description = "Cloud workspace for Vix projects.";
                          request.repository_url = "https://github.com/softadastra/cloud";
                          request.default_branch = "main";

                          auto created = service.create_project(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              created.value().owner_user_id,
                              std::string("user_123"));
                          Assert::equal(
                              created.value().name,
                              std::string("Softadastra Cloud"));
                          Assert::equal(
                              created.value().slug,
                              std::string("softadastra-cloud"));
                          Assert::equal(
                              created.value().default_branch,
                              std::string("main")); }));

  registry.add(TestCase("project service generates slug from name", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "Softadastra Cloud Core";

                          auto created = service.create_project(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().slug,
                              std::string("softadastra-cloud-core"));
                          Assert::equal(
                              created.value().default_branch,
                              std::string("main")); }));

  registry.add(TestCase("project service rejects duplicate slug in same workspace", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "Softadastra Cloud";
                          request.slug = "softadastra-cloud";

                          auto first = service.create_project(request);
                          auto second = service.create_project(request);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true); }));

  registry.add(TestCase("project service allows same slug in different workspaces", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest first;
                          first.workspace_id = "workspace_123";
                          first.owner_user_id = "user_123";
                          first.name = "Softadastra Cloud";
                          first.slug = "softadastra-cloud";

                          cloud::projects::dto::CreateProjectRequest second;
                          second.workspace_id = "workspace_456";
                          second.owner_user_id = "user_456";
                          second.name = "Softadastra Cloud";
                          second.slug = "softadastra-cloud";

                          auto first_created = service.create_project(first);
                          auto second_created = service.create_project(second);

                          Assert::equal(first_created.ok(), true);
                          Assert::equal(second_created.ok(), true); }));

  registry.add(TestCase("project service lists projects by workspace", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest first;
                          first.workspace_id = "workspace_123";
                          first.owner_user_id = "user_123";
                          first.name = "Cloud Core";
                          first.slug = "cloud-core";

                          cloud::projects::dto::CreateProjectRequest second;
                          second.workspace_id = "workspace_123";
                          second.owner_user_id = "user_123";
                          second.name = "Cloud Registry";
                          second.slug = "cloud-registry";

                          cloud::projects::dto::CreateProjectRequest third;
                          third.workspace_id = "workspace_456";
                          third.owner_user_id = "user_456";
                          third.name = "Other Project";
                          third.slug = "other-project";

                          service.create_project(first);
                          service.create_project(second);
                          service.create_project(third);

                          cloud::projects::dto::ListProjectsRequest list;
                          list.workspace_id = "workspace_123";

                          auto projects = service.list_projects(list);

                          Assert::equal(projects.ok(), true);
                          Assert::equal(
                              static_cast<int>(projects.value().size()),
                              2); }));

  registry.add(TestCase("project service finds project by id", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "Softadastra Cloud";
                          request.slug = "softadastra-cloud";

                          auto created = service.create_project(request);

                          cloud::projects::dto::ProjectLookupRequest lookup;
                          lookup.id = created.value().id;
                          lookup.workspace_id = "workspace_123";

                          auto found = service.find_project(lookup);

                          Assert::equal(found.ok(), true);
                          Assert::equal(
                              found.value().id,
                              created.value().id); }));

  registry.add(TestCase("project service updates project", []
                        {
                          cloud::projects::services::ProjectService service;

                          cloud::projects::dto::CreateProjectRequest create;
                          create.workspace_id = "workspace_123";
                          create.owner_user_id = "user_123";
                          create.name = "Softadastra Cloud";
                          create.slug = "softadastra-cloud";

                          auto created = service.create_project(create);

                          cloud::projects::dto::UpdateProjectRequest update;
                          update.id = created.value().id;
                          update.workspace_id = "workspace_123";
                          update.name = "Softadastra Cloud Core";
                          update.slug = "softadastra-cloud-core";
                          update.description = "Updated project.";
                          update.repository_url = "https://github.com/softadastra/cloud-core";
                          update.default_branch = "dev";

                          auto updated = service.update_project(update);

                          Assert::equal(updated.ok(), true);
                          Assert::equal(
                              updated.value().name,
                              std::string("Softadastra Cloud Core"));
                          Assert::equal(
                              updated.value().slug,
                              std::string("softadastra-cloud-core"));
                          Assert::equal(
                              updated.value().default_branch,
                              std::string("dev")); }));

  return TestRunner::run_all_and_exit();
}
