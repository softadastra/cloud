/**
 * @file test_tokens.cpp
 * @brief Basic tests for the tokens backend module.
 */

#include <tokens/TokensModule.hpp>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("tokens module exposes its name", []
  {
    Assert::equal(
        std::string(cloud::tokens::TokensModule::name()),
        std::string("tokens"));
  }));

  return TestRunner::run_all_and_exit();
}
