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

  return TestRunner::run_all_and_exit();
}
