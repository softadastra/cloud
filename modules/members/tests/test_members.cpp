/**
 * @file test_members.cpp
 * @brief Basic tests for the members backend module.
 */

#include <members/MembersModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("members module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::members::MembersModule::name()),
        std::string("members"));
  }));

  return TestRunner::run_all_and_exit();
}
