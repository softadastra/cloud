/**
 * @file test_build_reports.cpp
 * @brief Basic tests for the build_reports backend module.
 */

#include <build_reports/BuildReportsModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("build_reports module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::build_reports::BuildReportsModule::name()),
        std::string("build_reports"));
  }));

  return TestRunner::run_all_and_exit();
}
