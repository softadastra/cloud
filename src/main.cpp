/**
 *
 *  @file main.cpp
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

#include <exception>
#include <iostream>

#include <vix.hpp>
#include <vix/config/Config.hpp>
#include <vix/log/Log.hpp>

#include "app/AppFactory.hpp"
#include "config/CloudConfig.hpp"
#include "database/Database.hpp"
#include "database/Migrations.hpp"

namespace
{
  [[nodiscard]] int run_server()
  {
    vix::log::set_level(vix::log::LogLevel::Info);
    vix::log::set_format(vix::log::LogFormat::KV);

    vix::config::Config vix_config{".env"};

    const softadastra::cloud::config::CloudConfig cloud_config =
        softadastra::cloud::config::CloudConfig::from(vix_config);

    auto database =
        softadastra::cloud::database::Database::create(vix_config);

    softadastra::cloud::database::Migrations::run(*database);

    const softadastra::cloud::app::AppState state{
        cloud_config,
        database,
    };

    auto app =
        softadastra::cloud::app::AppFactory::create(state);

    vix::log::logf(
        vix::log::LogLevel::Info,
        "starting Softadastra Cloud",
        "service", cloud_config.service_name,
        "environment", cloud_config.environment,
        "port", cloud_config.server_port);

    app->run(cloud_config.server_port);

    return 0;
  }
}

int main()
{
  try
  {
    return run_server();
  }
  catch (const std::exception &e)
  {
    vix::log::logf(
        vix::log::LogLevel::Critical,
        "failed to start Softadastra Cloud",
        "error", e.what());

    std::cerr << "[softadastra-cloud] startup error: "
              << e.what()
              << "\n";

    return 1;
  }
  catch (...)
  {
    vix::log::critical("failed to start Softadastra Cloud: unknown error");

    std::cerr << "[softadastra-cloud] startup error: unknown error\n";

    return 1;
  }
}
