/**
 *
 *  @file Migrations.hpp
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

#ifndef SOFTADASTRA_CLOUD_DATABASE_MIGRATIONS_HPP
#define SOFTADASTRA_CLOUD_DATABASE_MIGRATIONS_HPP

#include <vix/db/db.hpp>

namespace softadastra::cloud::database
{
  /**
   * @brief Database migrations for Softadastra Cloud.
   *
   * Migrations creates the minimal database schema required by the backend.
   *
   * It is intentionally centralized at the beginning of the project so the
   * application can boot safely in local development before we introduce a
   * more advanced migration runner.
   */
  class Migrations final
  {
  public:
    /**
     * @brief Runs all database migrations.
     *
     * @param db Vix database instance.
     */
    static void run(vix::db::Database &db);

  private:
    Migrations() = delete;

    static void create_migrations_table(vix::db::Connection &conn);
    static void create_users_table(vix::db::Connection &conn);
    static void create_projects_table(vix::db::Connection &conn);
    static void create_agents_table(vix::db::Connection &conn);
    static void create_runs_table(vix::db::Connection &conn);
    static void create_run_events_table(vix::db::Connection &conn);
    static void create_reports_table(vix::db::Connection &conn);
    static void create_reliability_scores_table(vix::db::Connection &conn);
  };

} // namespace softadastra::cloud::database

#endif // SOFTADASTRA_CLOUD_DATABASE_MIGRATIONS_HPP
