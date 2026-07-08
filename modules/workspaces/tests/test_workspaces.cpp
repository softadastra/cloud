/**
 *
 *  @file test_workspaces.cpp
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
#include <workspaces/WorkspacesModule.hpp>
#include <workspaces/services/WorkspaceService.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("workspaces module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::workspaces::WorkspacesModule::name()),
                              std::string("workspaces")); }));

  registry.add(TestCase("workspace service creates a valid workspace", []
                        {
                          cloud::workspaces::services::WorkspaceService service;

                          cloud::workspaces::dto::CreateWorkspaceRequest request;
                          request.name = "Softadastra Cloud";
                          request.slug = "softadastra-cloud";
                          request.owner_user_id = "user_123";

                          auto created = service.create_workspace(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().name,
                              std::string("Softadastra Cloud"));
                          Assert::equal(
                              created.value().slug,
                              std::string("softadastra-cloud"));
                          Assert::equal(
                              created.value().owner_user_id,
                              std::string("user_123")); }));

  registry.add(TestCase("workspace service generates slug from name", []
                        {
                          cloud::workspaces::services::WorkspaceService service;

                          cloud::workspaces::dto::CreateWorkspaceRequest request;
                          request.name = "Softadastra Cloud Team";
                          request.owner_user_id = "user_123";

                          auto created = service.create_workspace(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().slug,
                              std::string("softadastra-cloud-team")); }));

  registry.add(TestCase("workspace service rejects duplicate slug", []
                        {
                          cloud::workspaces::services::WorkspaceService service;

                          cloud::workspaces::dto::CreateWorkspaceRequest request;
                          request.name = "Softadastra Cloud";
                          request.slug = "softadastra-cloud";
                          request.owner_user_id = "user_123";

                          auto first = service.create_workspace(request);
                          auto second = service.create_workspace(request);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true); }));

  registry.add(TestCase("workspace service lists workspaces by owner", []
                        {
                          cloud::workspaces::services::WorkspaceService service;

                          cloud::workspaces::dto::CreateWorkspaceRequest first;
                          first.name = "Cloud Core";
                          first.slug = "cloud-core";
                          first.owner_user_id = "user_123";

                          cloud::workspaces::dto::CreateWorkspaceRequest second;
                          second.name = "Cloud Registry";
                          second.slug = "cloud-registry";
                          second.owner_user_id = "user_123";

                          cloud::workspaces::dto::CreateWorkspaceRequest third;
                          third.name = "Other Workspace";
                          third.slug = "other-workspace";
                          third.owner_user_id = "user_456";

                          service.create_workspace(first);
                          service.create_workspace(second);
                          service.create_workspace(third);

                          auto workspaces = service.list_workspaces_for_owner("user_123");

                          Assert::equal(workspaces.ok(), true);
                          Assert::equal(
                              static_cast<int>(workspaces.value().size()),
                              2); }));

  return TestRunner::run_all_and_exit();
}
