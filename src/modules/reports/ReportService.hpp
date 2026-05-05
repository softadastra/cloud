/**
 *
 *  @file ReportService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_SERVICE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "app/AppState.hpp"
#include "modules/reports/Report.hpp"
#include "modules/reports/ReportRepository.hpp"
#include "modules/runs/RunRepository.hpp"

namespace softadastra::cloud::modules::reports
{
  /**
   * @brief Result returned by report operations.
   */
  struct ReportResult
  {
    bool success{false};
    int status{400};

    std::string error{"report_error"};
    std::string message{};

    Report report{};
  };

  /**
   * @brief List result returned by report listing.
   */
  struct ReportListResult
  {
    bool success{false};
    int status{400};

    std::string error{"report_error"};
    std::string message{};

    std::vector<Report> reports{};
    std::int64_t total{0};
    int limit{20};
    int offset{0};
  };

  /**
   * @brief Report business service.
   *
   * ReportService owns the business rules for run reports:
   * - validating report payloads
   * - checking run existence
   * - creating or updating reports
   * - listing reports
   * - loading reports by id or run id
   *
   * It does not read Vix Request and does not write Vix Response.
   */
  class ReportService final
  {
  public:
    /**
     * @brief Creates a report service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit ReportService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Creates or updates a report for a run.
     *
     * @param run_public_id Public run id.
     * @param summary Human-readable report summary.
     * @param raw_json Raw JSON report payload.
     * @return Report result.
     */
    [[nodiscard]] ReportResult submit_report(
        const std::string &run_public_id,
        const std::string &summary,
        const std::string &raw_json);

    /**
     * @brief Lists all reports.
     *
     * @param limit Max number of reports.
     * @param offset Pagination offset.
     * @return Report list result.
     */
    [[nodiscard]] ReportListResult list_reports(
        int limit,
        int offset);

    /**
     * @brief Gets one report by public id.
     *
     * @param public_id Public report id.
     * @return Report result.
     */
    [[nodiscard]] ReportResult get_report(
        const std::string &public_id);

    /**
     * @brief Gets one report by public run id.
     *
     * @param run_public_id Public run id.
     * @return Report result.
     */
    [[nodiscard]] ReportResult get_report_for_run(
        const std::string &run_public_id);

    /**
     * @brief Deletes one report by public id.
     *
     * @param public_id Public report id.
     * @return Report result.
     */
    [[nodiscard]] ReportResult delete_report(
        const std::string &public_id);

    /**
     * @brief Deletes a report attached to a run.
     *
     * @param run_public_id Public run id.
     * @return Report result.
     */
    [[nodiscard]] ReportResult delete_report_for_run(
        const std::string &run_public_id);

  private:
    const softadastra::cloud::app::AppState &state_;
    ReportRepository repository_;
    softadastra::cloud::modules::runs::RunRepository run_repository_;

    [[nodiscard]] static bool is_valid_summary(
        const std::string &summary) noexcept;

    [[nodiscard]] static bool is_valid_raw_json(
        const std::string &raw_json) noexcept;

    [[nodiscard]] static ReportResult validation_error(
        std::string message);

    [[nodiscard]] static ReportResult not_found(
        std::string message);

    [[nodiscard]] static ReportResult internal_error(
        std::string message);

    [[nodiscard]] static ReportListResult list_error(
        int status,
        std::string error,
        std::string message);
  };

} // namespace softadastra::cloud::modules::reports

#endif // SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_SERVICE_HPP
