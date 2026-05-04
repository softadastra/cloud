/**
 *
 *  @file Database.hpp
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

#ifndef SOFTADASTRA_CLOUD_DATABASE_DATABASE_HPP
#define SOFTADASTRA_CLOUD_DATABASE_DATABASE_HPP

#include <memory>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace softadastra::cloud::database
{
  /**
   * @brief Database factory for Softadastra Cloud.
   *
   * This class centralizes database creation so the rest of the backend does
   * not need to know whether the application is running with SQLite, MySQL,
   * or another Vix-supported database engine.
   *
   * The actual database settings are read from the Vix configuration.
   */
  class Database final
  {
  public:
    /**
     * @brief Creates a Vix database instance from application config.
     *
     * @param cfg Loaded Vix configuration.
     * @return Shared database instance.
     */
    [[nodiscard]] static std::shared_ptr<vix::db::Database> create(
        const vix::config::Config &cfg);

    /**
     * @brief Creates a local SQLite database instance.
     *
     * This helper is useful for tests, local development, and simple MVP
     * environments.
     *
     * @param path SQLite database file path.
     * @return Shared database instance.
     */
    [[nodiscard]] static std::shared_ptr<vix::db::Database> sqlite(
        const std::string &path);

  private:
    Database() = delete;
  };

} // namespace softadastra::cloud::database

#endif // SOFTADASTRA_CLOUD_DATABASE_DATABASE_HPP
