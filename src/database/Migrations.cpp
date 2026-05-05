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
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "name VARCHAR(255) NOT NULL UNIQUE, "
            "applied_at BIGINT NOT NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_users_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS users ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "public_id VARCHAR(64) NOT NULL UNIQUE, "
            "email VARCHAR(255) NOT NULL UNIQUE, "
            "password_hash VARCHAR(255) NOT NULL, "
            "name VARCHAR(255) NOT NULL, "
            "role VARCHAR(64) NOT NULL DEFAULT 'user', "
            "created_at BIGINT NOT NULL, "
            "updated_at BIGINT NOT NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_projects_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS projects ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "public_id VARCHAR(64) NOT NULL UNIQUE, "
            "user_id BIGINT NULL, "
            "name VARCHAR(255) NOT NULL, "
            "slug VARCHAR(255) NOT NULL UNIQUE, "
            "description TEXT NULL, "
            "base_url TEXT NULL, "
            "created_at BIGINT NOT NULL, "
            "updated_at BIGINT NOT NULL, "
            "CONSTRAINT fk_projects_user "
            "FOREIGN KEY(user_id) REFERENCES users(id) "
            "ON DELETE SET NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_agents_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS agents ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "public_id VARCHAR(64) NOT NULL UNIQUE, "
            "project_id BIGINT NOT NULL, "
            "name VARCHAR(255) NOT NULL, "
            "api_key_hash VARCHAR(255) NOT NULL, "
            "last_seen_at BIGINT NULL, "
            "created_at BIGINT NOT NULL, "
            "updated_at BIGINT NOT NULL, "
            "CONSTRAINT fk_agents_project "
            "FOREIGN KEY(project_id) REFERENCES projects(id) "
            "ON DELETE CASCADE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_runs_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS runs ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "public_id VARCHAR(64) NOT NULL UNIQUE, "
            "project_id BIGINT NOT NULL, "
            "agent_id BIGINT NULL, "
            "status VARCHAR(64) NOT NULL DEFAULT 'created', "
            "target_url TEXT NOT NULL, "
            "method VARCHAR(16) NOT NULL DEFAULT 'GET', "
            "scenario VARCHAR(255) NOT NULL, "
            "started_at BIGINT NOT NULL, "
            "finished_at BIGINT NULL, "
            "created_at BIGINT NOT NULL, "
            "updated_at BIGINT NOT NULL, "
            "CONSTRAINT fk_runs_project "
            "FOREIGN KEY(project_id) REFERENCES projects(id) "
            "ON DELETE CASCADE, "
            "CONSTRAINT fk_runs_agent "
            "FOREIGN KEY(agent_id) REFERENCES agents(id) "
            "ON DELETE SET NULL"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_run_events_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS run_events ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "public_id VARCHAR(64) NOT NULL UNIQUE, "
            "run_id BIGINT NOT NULL, "
            "event_type VARCHAR(128) NOT NULL, "
            "severity VARCHAR(32) NOT NULL DEFAULT 'info', "
            "message TEXT NULL, "
            "payload LONGTEXT NULL, "
            "created_at BIGINT NOT NULL, "
            "CONSTRAINT fk_run_events_run "
            "FOREIGN KEY(run_id) REFERENCES runs(id) "
            "ON DELETE CASCADE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_reports_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS reports ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "public_id VARCHAR(64) NOT NULL UNIQUE, "
            "run_id BIGINT NOT NULL UNIQUE, "
            "summary TEXT NOT NULL, "
            "raw_json LONGTEXT NOT NULL, "
            "created_at BIGINT NOT NULL, "
            "updated_at BIGINT NOT NULL, "
            "CONSTRAINT fk_reports_run "
            "FOREIGN KEY(run_id) REFERENCES runs(id) "
            "ON DELETE CASCADE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

  void Migrations::create_reliability_scores_table(vix::db::Connection &conn)
  {
    conn.prepare(
            "CREATE TABLE IF NOT EXISTS reliability_scores ("
            "id BIGINT PRIMARY KEY AUTO_INCREMENT, "
            "run_id BIGINT NOT NULL UNIQUE, "
            "score INT NOT NULL, "
            "grade VARCHAR(8) NOT NULL, "
            "data_loss_risk TINYINT(1) NOT NULL DEFAULT 0, "
            "duplicate_risk TINYINT(1) NOT NULL DEFAULT 0, "
            "timeout_risk TINYINT(1) NOT NULL DEFAULT 0, "
            "recovery_risk TINYINT(1) NOT NULL DEFAULT 0, "
            "created_at BIGINT NOT NULL, "
            "CONSTRAINT fk_reliability_scores_run "
            "FOREIGN KEY(run_id) REFERENCES runs(id) "
            "ON DELETE CASCADE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
        ->exec();
  }

} // namespace softadastra::cloud::database
