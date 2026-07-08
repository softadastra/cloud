/**
 * @file test_projects.cpp
 * @brief Basic tests for the projects backend module.
 */

#include <projects/ProjectsModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("projects module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::projects::ProjectsModule::name()),
        std::string("projects"));
  }));

  return TestRunner::run_all_and_exit();
}
