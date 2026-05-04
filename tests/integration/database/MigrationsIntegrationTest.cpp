/**
 *
 *  @file MigrationsIntegrationTest.cpp
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

#include <filesystem>
#include <string>

#include <vix/db/db.hpp>
#include <vix/tests/tests.hpp>

#include "database/Migrations.hpp"

namespace
{
  [[nodiscard]] std::filesystem::path test_database_path()
  {
    return std::filesystem::temp_directory_path() /
           "softadastra_cloud_migrations_test.db";
  }

  void remove_test_database()
  {
    std::error_code ec;
    std::filesystem::remove(test_database_path(), ec);
  }

  [[nodiscard]] vix::db::Database make_database()
  {
    return vix::db::Database::sqlite(
        test_database_path().string());
  }

  [[nodiscard]] bool table_exists(
      vix::db::Database &database,
      const std::string &table_name)
  {
    auto rows =
        database.query(
            "SELECT name "
            "FROM sqlite_master "
            "WHERE type = 'table' "
            "AND name = ? "
            "LIMIT 1",
            table_name);

    return rows->next();
  }

  void register_migrations_integration_tests()
  {
    vix::tests::TestSuite suite{"integration.database.Migrations"};

    suite.add(
        vix::tests::TestCase{
            "migrations create expected tables",
            []
            {
              remove_test_database();

              auto database = make_database();

              softadastra::cloud::database::Migrations::run(database);

              vix::tests::Assert::is_true(
                  table_exists(database, "migrations"),
                  "migrations table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "users"),
                  "users table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "projects"),
                  "projects table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "agents"),
                  "agents table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "runs"),
                  "runs table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "run_events"),
                  "run_events table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "reports"),
                  "reports table should exist");

              vix::tests::Assert::is_true(
                  table_exists(database, "reliability_scores"),
                  "reliability_scores table should exist");

              remove_test_database();
            }});

    suite.add(
        vix::tests::TestCase{
            "migrations are idempotent",
            []
            {
              remove_test_database();

              auto database = make_database();

              softadastra::cloud::database::Migrations::run(database);
              softadastra::cloud::database::Migrations::run(database);

              vix::tests::Assert::is_true(
                  table_exists(database, "migrations"),
                  "migrations table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "users"),
                  "users table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "projects"),
                  "projects table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "agents"),
                  "agents table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "runs"),
                  "runs table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "run_events"),
                  "run_events table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "reports"),
                  "reports table should still exist after second run");

              vix::tests::Assert::is_true(
                  table_exists(database, "reliability_scores"),
                  "reliability_scores table should still exist after second run");

              remove_test_database();
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_migrations_integration_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
