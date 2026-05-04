/**
 *
 *  @file ReportRoutes.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_ROUTES_HPP
#define SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_ROUTES_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::reports
{
  /**
   * @brief Registers report routes.
   *
   * ReportRoutes only declares HTTP paths and forwards requests to
   * ReportController.
   */
  class ReportRoutes final
  {
  public:
    /**
     * @brief Registers report routes on the Vix app.
     *
     * @param app Vix application.
     * @param state Shared application state.
     */
    static void register_routes(
        vix::App &app,
        const softadastra::cloud::app::AppState &state);

  private:
    ReportRoutes() = delete;
  };

} // namespace softadastra::cloud::modules::reports

#endif // SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_ROUTES_HPP
