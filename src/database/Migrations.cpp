/*
 * Migrations.cpp
 */

#include "database/Migrations.hpp"

#include <vix/log/Log.hpp>

namespace softadastra::cloud::database
{
  void Migrations::run(vix::db::Database &db)
  {
    vix::log::info("running database migrations");

    db.transaction(
        [](vix::db::Connection &conn)
        {
          create_migrations_table(conn);
          create_users_table(conn);
          create_projects_table(conn);
          create_agents_table(conn);
          create_runs_table(conn);
          create_run_events_table(conn);
          create_reports_table(conn);
          create_reliability_scores_table(conn);
        });

    vix::log::info("database migrations completed");
  }

  void Migrations::create_migrations_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS migrations ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL UNIQUE, "
            "applied_at INTEGER NOT NULL)")
        ->exec();
  }

  void Migrations::create_users_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "public_id TEXT NOT NULL UNIQUE, "
            "email TEXT NOT NULL UNIQUE, "
            "password_hash TEXT NOT NULL, "
            "name TEXT NOT NULL, "
            "role TEXT NOT NULL DEFAULT 'user', "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL)")
        ->exec();
  }

  void Migrations::create_projects_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS projects ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "public_id TEXT NOT NULL UNIQUE, "
            "user_id INTEGER, "
            "name TEXT NOT NULL, "
            "slug TEXT NOT NULL UNIQUE, "
            "description TEXT, "
            "base_url TEXT, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL, "
            "FOREIGN KEY(user_id) REFERENCES users(id))")
        ->exec();
  }

  void Migrations::create_agents_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS agents ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "public_id TEXT NOT NULL UNIQUE, "
            "project_id INTEGER NOT NULL, "
            "name TEXT NOT NULL, "
            "api_key_hash TEXT NOT NULL, "
            "last_seen_at INTEGER, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL, "
            "FOREIGN KEY(project_id) REFERENCES projects(id))")
        ->exec();
  }

  void Migrations::create_runs_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS runs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "public_id TEXT NOT NULL UNIQUE, "
            "project_id INTEGER NOT NULL, "
            "agent_id INTEGER, "
            "status TEXT NOT NULL DEFAULT 'created', "
            "target_url TEXT NOT NULL, "
            "method TEXT NOT NULL DEFAULT 'GET', "
            "scenario TEXT NOT NULL, "
            "started_at INTEGER NOT NULL, "
            "finished_at INTEGER, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL, "
            "FOREIGN KEY(project_id) REFERENCES projects(id), "
            "FOREIGN KEY(agent_id) REFERENCES agents(id))")
        ->exec();
  }

  void Migrations::create_run_events_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS run_events ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "public_id TEXT NOT NULL UNIQUE, "
            "run_id INTEGER NOT NULL, "
            "event_type TEXT NOT NULL, "
            "severity TEXT NOT NULL DEFAULT 'info', "
            "message TEXT, "
            "payload TEXT, "
            "created_at INTEGER NOT NULL, "
            "FOREIGN KEY(run_id) REFERENCES runs(id))")
        ->exec();
  }

  void Migrations::create_reports_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS reports ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "public_id TEXT NOT NULL UNIQUE, "
            "run_id INTEGER NOT NULL UNIQUE, "
            "summary TEXT NOT NULL, "
            "raw_json TEXT NOT NULL, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL, "
            "FOREIGN KEY(run_id) REFERENCES runs(id))")
        ->exec();
  }

  void Migrations::create_reliability_scores_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS reliability_scores ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "run_id INTEGER NOT NULL UNIQUE, "
            "score INTEGER NOT NULL, "
            "grade TEXT NOT NULL, "
            "data_loss_risk INTEGER NOT NULL DEFAULT 0, "
            "duplicate_risk INTEGER NOT NULL DEFAULT 0, "
            "timeout_risk INTEGER NOT NULL DEFAULT 0, "
            "recovery_risk INTEGER NOT NULL DEFAULT 0, "
            "created_at INTEGER NOT NULL, "
            "FOREIGN KEY(run_id) REFERENCES runs(id))")
        ->exec();
  }

} // namespace softadastra::cloud::database
