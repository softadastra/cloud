/*
 * Database.cpp
 */

#include "database/Database.hpp"

#include <memory>
#include <stdexcept>
#include <string>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::database
{
  namespace
  {
    [[nodiscard]] std::string database_engine(
        const vix::config::Config &cfg)
    {
      return cfg.getString("database.engine", "sqlite");
    }

    [[nodiscard]] std::string sqlite_path(
        const vix::config::Config &cfg)
    {
      return cfg.getString(
          "database.sqlite.path",
          cfg.getString("database.default.name", "softadastra-cloud.db"));
    }
  }

  std::shared_ptr<vix::db::Database> Database::create(
      const vix::config::Config &cfg)
  {
    const std::string engine = database_engine(cfg);

    vix::log::logf(
        vix::log::LogLevel::Info,
        "initializing database",
        "engine", engine);

    try
    {
      auto db = std::make_shared<vix::db::Database>(cfg);

      vix::log::logf(
          vix::log::LogLevel::Info,
          "database initialized",
          "engine", engine);

      return db;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to initialize database from config",
          "engine", engine,
          "error", e.what());

      if (engine == "sqlite")
      {
        const std::string path = sqlite_path(cfg);

        vix::log::logf(
            vix::log::LogLevel::Warn,
            "falling back to explicit SQLite database",
            "path", path);

        return sqlite(path);
      }

      throw;
    }
  }

  std::shared_ptr<vix::db::Database> Database::sqlite(
      const std::string &path)
  {
    if (path.empty())
    {
      throw std::invalid_argument(
          "SQLite database path cannot be empty");
    }

    vix::log::logf(
        vix::log::LogLevel::Info,
        "initializing SQLite database",
        "path", path);

    auto db =
        std::make_shared<vix::db::Database>(
            vix::db::Database::sqlite(path));

    vix::log::logf(
        vix::log::LogLevel::Info,
        "SQLite database initialized",
        "path", path);

    return db;
  }

} // namespace softadastra::cloud::database
