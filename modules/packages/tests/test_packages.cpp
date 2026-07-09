/**
 *
 *  @file test_packages.cpp
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
#include <packages/PackagesModule.hpp>
#include <packages/services/PackageService.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("packages module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::packages::PackagesModule::name()),
                              std::string("packages")); }));

  registry.add(TestCase("package service creates a valid package", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "vix/http";
                          request.description = "HTTP helpers for Vix applications.";
                          request.repository_url = "https://github.com/softadastra/vix-http";
                          request.visibility = "private";

                          auto created = service.create_package(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              created.value().owner_user_id,
                              std::string("user_123"));
                          Assert::equal(
                              created.value().name,
                              std::string("vix/http"));
                          Assert::equal(
                              created.value().visibility,
                              std::string("private")); }));

  registry.add(TestCase("package service uses private visibility by default", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "vix/config";

                          auto created = service.create_package(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().visibility,
                              std::string("private")); }));

  registry.add(TestCase("package service rejects invalid visibility", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "vix/http";
                          request.visibility = "internal";

                          auto created = service.create_package(request);

                          Assert::equal(created.failed(), true); }));

  registry.add(TestCase("package service rejects duplicate package in same workspace", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "vix/http";
                          request.visibility = "private";

                          auto first = service.create_package(request);
                          auto second = service.create_package(request);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true); }));

  registry.add(TestCase("package service allows same package name in different workspaces", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest first;
                          first.workspace_id = "workspace_123";
                          first.owner_user_id = "user_123";
                          first.name = "vix/http";
                          first.visibility = "private";

                          cloud::packages::dto::CreatePackageRequest second;
                          second.workspace_id = "workspace_456";
                          second.owner_user_id = "user_456";
                          second.name = "vix/http";
                          second.visibility = "private";

                          auto first_created = service.create_package(first);
                          auto second_created = service.create_package(second);

                          Assert::equal(first_created.ok(), true);
                          Assert::equal(second_created.ok(), true); }));

  registry.add(TestCase("package service lists packages by workspace", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest first;
                          first.workspace_id = "workspace_123";
                          first.owner_user_id = "user_123";
                          first.name = "vix/http";
                          first.visibility = "private";

                          cloud::packages::dto::CreatePackageRequest second;
                          second.workspace_id = "workspace_123";
                          second.owner_user_id = "user_123";
                          second.name = "vix/config";
                          second.visibility = "private";

                          cloud::packages::dto::CreatePackageRequest third;
                          third.workspace_id = "workspace_456";
                          third.owner_user_id = "user_456";
                          third.name = "vix/other";
                          third.visibility = "private";

                          service.create_package(first);
                          service.create_package(second);
                          service.create_package(third);

                          cloud::packages::dto::ListPackagesRequest list;
                          list.workspace_id = "workspace_123";

                          auto packages = service.list_packages(list);

                          Assert::equal(packages.ok(), true);
                          Assert::equal(
                              static_cast<int>(packages.value().size()),
                              2); }));

  registry.add(TestCase("package service finds package by id", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest request;
                          request.workspace_id = "workspace_123";
                          request.owner_user_id = "user_123";
                          request.name = "vix/http";
                          request.visibility = "private";

                          auto created = service.create_package(request);

                          cloud::packages::dto::PackageLookupRequest lookup;
                          lookup.id = created.value().id;
                          lookup.workspace_id = "workspace_123";

                          auto found = service.find_package(lookup);

                          Assert::equal(found.ok(), true);
                          Assert::equal(
                              found.value().id,
                              created.value().id); }));

  registry.add(TestCase("package service updates package", []
                        {
                          cloud::packages::services::PackageService service;

                          cloud::packages::dto::CreatePackageRequest create;
                          create.workspace_id = "workspace_123";
                          create.owner_user_id = "user_123";
                          create.name = "vix/http";
                          create.description = "HTTP helpers.";
                          create.repository_url = "https://github.com/softadastra/vix-http";
                          create.visibility = "private";

                          auto created = service.create_package(create);

                          cloud::packages::dto::UpdatePackageRequest update;
                          update.id = created.value().id;
                          update.workspace_id = "workspace_123";
                          update.name = "vix/http-core";
                          update.description = "Updated HTTP helpers.";
                          update.repository_url = "https://github.com/softadastra/vix-http-core";
                          update.visibility = "public";

                          auto updated = service.update_package(update);

                          Assert::equal(updated.ok(), true);
                          Assert::equal(
                              updated.value().name,
                              std::string("vix/http-core"));
                          Assert::equal(
                              updated.value().visibility,
                              std::string("public"));
                          Assert::equal(
                              updated.value().description,
                              std::string("Updated HTTP helpers.")); }));

  return TestRunner::run_all_and_exit();
}
