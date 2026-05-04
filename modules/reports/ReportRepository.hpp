/**
 *
 *  @file ReportRepository.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_REPOSITORY_HPP
#define SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vix/db/db.hpp>

#include "modules/reports/Report.hpp"

namespace softadastra::cloud::modules::reports
{
  /**
   * @brief Repository for report persistence.
   *
   * ReportRepository owns all SQL access for reports.
   *
   * It does not contain HTTP logic or business rules. Those belong to
   * ReportController and ReportService.
   */
  class ReportRepository final
  {
  public:
    /**
     * @brief Creates a report repository.
     *
     * @param database Shared Vix database instance.
     */
    explicit ReportRepository(
        std::shared_ptr<vix::db::Database> database);

    /**
     * @brief Creates a report for a run.
     *
     * @param public_id Public report id.
     * @param run_id Internal run id.
     * @param summary Human-readable report summary.
     * @param raw_json Raw JSON report payload.
     * @param now_ms Creation/update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool create(
        const std::string &public_id,
        std::int64_t run_id,
        const std::string &summary,
        const std::string &raw_json,
        std::int64_t now_ms);

    /**
     * @brief Returns all reports.
     *
     * @param limit Max number of reports.
     * @param offset Offset for pagination.
     * @return Report list.
     */
    [[nodiscard]] std::vector<Report> all(
        int limit,
        int offset) const;

    /**
     * @brief Finds one report by internal id.
     *
     * @param id Internal database id.
     * @return Report if found.
     */
    [[nodiscard]] std::optional<Report> find_by_id(
        std::int64_t id) const;

    /**
     * @brief Finds one report by public id.
     *
     * @param public_id Public report id.
     * @return Report if found.
     */
    [[nodiscard]] std::optional<Report> find_by_public_id(
        const std::string &public_id) const;

    /**
     * @brief Finds one report by public run id.
     *
     * @param run_public_id Public run id.
     * @return Report if found.
     */
    [[nodiscard]] std::optional<Report> find_by_run_public_id(
        const std::string &run_public_id) const;

    /**
     * @brief Returns true when a report already exists for a run.
     *
     * @param run_public_id Public run id.
     * @return true if a report exists.
     */
    [[nodiscard]] bool exists_for_run(
        const std::string &run_public_id) const;

    /**
     * @brief Updates an existing report for a run.
     *
     * @param run_public_id Public run id.
     * @param summary Human-readable report summary.
     * @param raw_json Raw JSON report payload.
     * @param now_ms Update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool update_for_run(
        const std::string &run_public_id,
        const std::string &summary,
        const std::string &raw_json,
        std::int64_t now_ms);

    /**
     * @brief Deletes a report by public id.
     *
     * @param public_id Public report id.
     * @return true on success.
     */
    [[nodiscard]] bool remove(
        const std::string &public_id);

    /**
     * @brief Deletes a report for a run.
     *
     * Useful when deleting a run.
     *
     * @param run_public_id Public run id.
     * @return true on success.
     */
    [[nodiscard]] bool remove_for_run(
        const std::string &run_public_id);

    /**
     * @brief Counts all reports.
     *
     * @return Report count.
     */
    [[nodiscard]] std::int64_t count() const;

  private:
    std::shared_ptr<vix::db::Database> database_;

    [[nodiscard]] static Report report_from_current_row(
        const vix::db::Row &row);

    [[nodiscard]] static int normalize_limit(int limit) noexcept;
    [[nodiscard]] static int normalize_offset(int offset) noexcept;
  };

} // namespace softadastra::cloud::modules::reports

#endif // SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_REPOSITORY_HPP
