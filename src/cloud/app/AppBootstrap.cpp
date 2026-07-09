/**
 * @file AppBootstrap.cpp
 * @brief Startup implementation for the cloud backend.
 */

#include <cloud/app/AppBootstrap.hpp>
#include <cloud/presentation/middleware/MiddlewareRegistry.hpp>
#include <cloud/presentation/routes/RouteRegistry.hpp>

#include <vix_app_modules.hpp>

#include <vix.hpp>
#include <vix/db/db.hpp>
#include <vix/executor/RuntimeExecutor.hpp>
#include <vix/log.hpp>

#include <cstdlib>
#include <filesystem>
#include <memory>

namespace cloud::app
{
  namespace
  {
    void initialize_database(vix::config::Config &cfg)
    {
      const std::filesystem::path migrations_dir{
          cfg.getString("orm.dir", "migrations")};

      vix::log::info("Applying database migrations from {}",
                     migrations_dir.string());

      vix::db::Database db{cfg};
      vix::db::PooledConn conn{db.pool()};

      vix::db::FileMigrationsRunner runner(*conn, migrations_dir);
      runner.setTable("schema_migrations");
      runner.applyAll();

      vix::log::info("Database migrations are up to date");

#ifdef _WIN32
      _putenv_s("SOFTADASTRA_CLOUD_USE_DB", "1");
#else
      setenv("SOFTADASTRA_CLOUD_USE_DB", "1", 1);
#endif
    }
  } // namespace

  int AppBootstrap::run()
  {
    vix::config::Config cfg{".env"};

    initialize_database(cfg);

    auto executor = std::make_shared<vix::executor::RuntimeExecutor>(1u);
    vix::App app{executor};

    presentation::middleware::MiddlewareRegistry::register_all(app);
    presentation::routes::RouteRegistry::register_all(app);
    vix::app_generated::register_app_modules(app);

    vix::log::info("Starting cloud on port {}", cfg.getServerPort());

    return vix::app_generated::run_app(app, cfg, executor);
  }
} // namespace cloud::app
