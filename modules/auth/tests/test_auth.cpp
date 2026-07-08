/**
 *
 *  @file test_auth.cpp
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
#include <auth/AuthModule.hpp>
#include <auth/services/AuthService.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("auth module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::auth::AuthModule::name()),
                              std::string("auth")); }));

  registry.add(TestCase("auth service registers a valid user", []
                        {
                          cloud::auth::services::AuthService service;

                          cloud::auth::dto::RegisterRequest request;
                          request.name = "Ada Lovelace";
                          request.email = "ada@example.com";
                          request.password = "correct-password";

                          auto registered = service.register_user(request);

                          Assert::equal(registered.ok(), true);
                          Assert::equal(
                              registered.value().email(),
                              std::string("ada@example.com")); }));

  registry.add(TestCase("auth service rejects duplicate users", []
                        {
                          cloud::auth::services::AuthService service;

                          cloud::auth::dto::RegisterRequest request;
                          request.name = "Ada Lovelace";
                          request.email = "ada@example.com";
                          request.password = "correct-password";

                          auto first = service.register_user(request);
                          auto second = service.register_user(request);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true); }));

  return TestRunner::run_all_and_exit();
}
