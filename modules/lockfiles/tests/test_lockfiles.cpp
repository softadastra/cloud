/**
 * @file test_lockfiles.cpp
 * @brief Basic tests for the lockfiles backend module.
 */

#include <lockfiles/LockfilesModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("lockfiles module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::lockfiles::LockfilesModule::name()),
        std::string("lockfiles"));
  }));

  return TestRunner::run_all_and_exit();
}
