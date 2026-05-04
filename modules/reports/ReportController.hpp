/**
 *
 *  @file ReportController.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_CONTROLLER_HPP
#define SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_CONTROLLER_HPP

#include <vix.hpp>

#include "app/AppState.hpp"

namespace softadastra::cloud::modules::reports
{
  /**
   * @brief HTTP controller for report endpoints.
   *
   * ReportController handles only HTTP concerns:
   * - read path/query parameters
   * - read JSON request body
   * - call ReportService
   * - return standardized JSON responses
   *
   * Business rules belong in ReportService.
   * SQL belongs in ReportRepository.
   */
  class ReportController final
  {
  public:
    /**
     * @brief Submits a report for a run.
     *
     * Path:
     * /api/runs/{id}/report
     *
     * Expected JSON body:
     * {
     *   "summary": "Run completed with timeout risk",
     *   "report": { ... }
     * }
     *
     * The "report" field can be an object, array, or string.
     * Non-string values are serialized and stored as raw_json.
     */
    static void submit_for_run(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Lists reports.
     *
     * Query params:
     * - limit
     * - offset
     */
    static void list(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Gets one report by public id.
     *
     * Path:
     * /api/reports/{id}
     */
    static void get(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Gets the report attached to one run.
     *
     * Path:
     * /api/runs/{id}/report
     */
    static void get_for_run(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Deletes one report by public id.
     *
     * Path:
     * /api/reports/{id}
     */
    static void remove(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Deletes the report attached to one run.
     *
     * Path:
     * /api/runs/{id}/report
     */
    static void remove_for_run(
        vix::Request &req,
        vix::Response &res,
        const softadastra::cloud::app::AppState &state);

  private:
    ReportController() = delete;
  };

} // namespace softadastra::cloud::modules::reports

#endif // SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_CONTROLLER_HPP
