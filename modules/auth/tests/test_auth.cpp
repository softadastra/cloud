/**
 * @file test_auth.cpp
 * @brief Basic tests for the auth backend module.
 */

#include <auth/AuthModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("auth module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::auth::AuthModule::name()),
        std::string("auth"));
  }));

  return TestRunner::run_all_and_exit();
}
