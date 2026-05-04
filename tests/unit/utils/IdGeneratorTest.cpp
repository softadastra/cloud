/**
 *
 *  @file IdGeneratorTest.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra/cloud
 *
 *  Licensed under the Apache License, Version 2.0.
 *
 *  Softadastra Cloud
 *
 */

#include <cstddef>
#include <set>
#include <string>

#include <vix/tests/tests.hpp>

#include "utils/IdGenerator.hpp"

namespace
{
  [[nodiscard]] bool starts_with(
      const std::string &value,
      const std::string &prefix)
  {
    return value.rfind(prefix, 0) == 0;
  }

  [[nodiscard]] bool is_lower_hex(char c) noexcept
  {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f');
  }

  void register_id_generator_tests()
  {
    vix::tests::TestSuite suite{"utils.IdGenerator"};

    suite.add(
        vix::tests::TestCase{
            "generate uses valid prefix",
            []
            {
              const std::string id =
                  softadastra::cloud::utils::IdGenerator::generate("run");

              vix::tests::Assert::is_true(
                  starts_with(id, "run_"),
                  "generated id should start with run_");
            }});

    suite.add(
        vix::tests::TestCase{
            "generate falls back for invalid prefix",
            []
            {
              const std::string id =
                  softadastra::cloud::utils::IdGenerator::generate("Run_Invalid");

              vix::tests::Assert::is_true(
                  starts_with(id, "id_"),
                  "invalid prefix should fall back to id_");
            }});

    suite.add(
        vix::tests::TestCase{
            "generated suffix is lowercase hex",
            []
            {
              const std::string id =
                  softadastra::cloud::utils::IdGenerator::generate("evt");

              const std::string prefix = "evt_";

              vix::tests::Assert::is_true(
                  starts_with(id, prefix),
                  "generated id should start with evt_");

              vix::tests::Assert::equal(
                  prefix.size() + 32,
                  id.size(),
                  "generated id should contain a 32-character suffix");

              for (std::size_t i = prefix.size(); i < id.size(); ++i)
              {
                vix::tests::Assert::is_true(
                    is_lower_hex(id[i]),
                    "suffix should contain only lowercase hex characters");
              }
            }});

    suite.add(
        vix::tests::TestCase{
            "random_suffix returns 32 lowercase hex characters",
            []
            {
              const std::string suffix =
                  softadastra::cloud::utils::IdGenerator::random_suffix();

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(32),
                  suffix.size(),
                  "random suffix should contain 32 characters");

              for (const char c : suffix)
              {
                vix::tests::Assert::is_true(
                    is_lower_hex(c),
                    "random suffix should contain only lowercase hex characters");
              }
            }});

    suite.add(
        vix::tests::TestCase{
            "two generated ids are different",
            []
            {
              const std::string first =
                  softadastra::cloud::utils::IdGenerator::generate("prj");

              const std::string second =
                  softadastra::cloud::utils::IdGenerator::generate("prj");

              vix::tests::Assert::not_equal(
                  first,
                  second,
                  "two generated ids should not be equal");
            }});

    suite.add(
        vix::tests::TestCase{
            "many generated ids are unique",
            []
            {
              std::set<std::string> ids;

              for (int i = 0; i < 1000; ++i)
              {
                ids.insert(
                    softadastra::cloud::utils::IdGenerator::generate("agt"));
              }

              vix::tests::Assert::equal(
                  static_cast<std::size_t>(1000),
                  ids.size(),
                  "1000 generated ids should be unique");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_id_generator_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
