/**
 * @file test_packages.cpp
 * @brief Basic tests for the packages backend module.
 */

#include <packages/PackagesModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("packages module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::packages::PackagesModule::name()),
        std::string("packages"));
  }));

  return TestRunner::run_all_and_exit();
}
