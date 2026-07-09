/**
 *
 *  @file BuildReportsModule.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Softadastra Cloud
 */
#include <build_reports/BuildReportsModule.hpp>
#include <build_reports/controllers/BuildReportsController.hpp>

#include <vix.hpp>

namespace cloud::build_reports
{
  const char *BuildReportsModule::name()
  {
    return "build_reports";
  }

  void BuildReportsModule::register_routes(vix::App &app)
  {
    controllers::BuildReportsController::register_routes(app);
  }
} // namespace cloud::build_reports
