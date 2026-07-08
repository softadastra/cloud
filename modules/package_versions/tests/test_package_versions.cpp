/**
 * @file test_package_versions.cpp
 * @brief Basic tests for the package_versions backend module.
 */

#include <package_versions/PackageVersionsModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("package_versions module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::package_versions::PackageVersionsModule::name()),
        std::string("package_versions"));
  }));

  return TestRunner::run_all_and_exit();
}
