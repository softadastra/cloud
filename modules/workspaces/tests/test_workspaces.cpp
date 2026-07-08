/**
 * @file test_workspaces.cpp
 * @brief Basic tests for the workspaces backend module.
 */

#include <workspaces/WorkspacesModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("workspaces module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::workspaces::WorkspacesModule::name()),
        std::string("workspaces"));
  }));

  return TestRunner::run_all_and_exit();
}
