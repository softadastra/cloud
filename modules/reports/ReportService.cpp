/*
 * ReportService.cpp
 */

#include "modules/reports/ReportService.hpp"

#include <algorithm>
#include <string>

#include "utils/IdGenerator.hpp"
#include "utils/Time.hpp"

namespace softadastra::cloud::modules::reports
{
  namespace
  {
    constexpr std::size_t MIN_SUMMARY_LENGTH = 2;
    constexpr std::size_t MAX_SUMMARY_LENGTH = 2000;
    constexpr std::size_t MAX_RAW_JSON_LENGTH = 5 * 1024 * 1024;
  }

  ReportService::ReportService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        repository_(state.database),
        run_repository_(state.database)
  {
  }

  ReportResult ReportService::submit_report(
      const std::string &run_public_id,
      const std::string &summary,
      const std::string &raw_json)
  {
    if (run_public_id.empty())
    {
      return validation_error("Run id is required");
    }

    if (!is_valid_summary(summary))
    {
      return validation_error(
          "Report summary must contain between 2 and 2000 characters");
    }

    if (!is_valid_raw_json(raw_json))
    {
      return validation_error(
          "Report raw JSON is required and must contain at most 5MB");
    }

    const auto run =
        run_repository_.find_by_public_id(run_public_id);

    if (!run.has_value())
    {
      return not_found("Run not found");
    }

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    bool saved = false;

    if (repository_.exists_for_run(run_public_id))
    {
      saved =
          repository_.update_for_run(
              run_public_id,
              summary,
              raw_json,
              now);
    }
    else
    {
      const std::string public_id =
          softadastra::cloud::utils::IdGenerator::generate("rpt");

      saved =
          repository_.create(
              public_id,
              run->id,
              summary,
              raw_json,
              now);
    }

    if (!saved)
    {
      return internal_error("Failed to save report");
    }

    auto report =
        repository_.find_by_run_public_id(run_public_id);

    if (!report.has_value())
    {
      return internal_error("Report was saved but could not be loaded");
    }

    ReportResult result;
    result.success = true;
    result.status = repository_.exists_for_run(run_public_id) ? 200 : 201;
    result.error.clear();
    result.message = "Report saved";
    result.report = *report;

    return result;
  }

  ReportListResult ReportService::list_reports(
      int limit,
      int offset)
  {
    ReportListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Reports loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.reports =
        repository_.all(
            result.limit,
            result.offset);
    result.total = repository_.count();

    return result;
  }

  ReportResult ReportService::get_report(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Report id is required");
    }

    auto report =
        repository_.find_by_public_id(public_id);

    if (!report.has_value())
    {
      return not_found("Report not found");
    }

    ReportResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Report loaded";
    result.report = *report;

    return result;
  }

  ReportResult ReportService::get_report_for_run(
      const std::string &run_public_id)
  {
    if (run_public_id.empty())
    {
      return validation_error("Run id is required");
    }

    const auto run =
        run_repository_.find_by_public_id(run_public_id);

    if (!run.has_value())
    {
      return not_found("Run not found");
    }

    auto report =
        repository_.find_by_run_public_id(run_public_id);

    if (!report.has_value())
    {
      return not_found("Report not found");
    }

    ReportResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Report loaded";
    result.report = *report;

    return result;
  }

  ReportResult ReportService::delete_report(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Report id is required");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Report not found");
    }

    const bool deleted =
        repository_.remove(public_id);

    if (!deleted)
    {
      return internal_error("Failed to delete report");
    }

    ReportResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Report deleted";
    result.report = *existing;

    return result;
  }

  ReportResult ReportService::delete_report_for_run(
      const std::string &run_public_id)
  {
    if (run_public_id.empty())
    {
      return validation_error("Run id is required");
    }

    auto existing =
        repository_.find_by_run_public_id(run_public_id);

    if (!existing.has_value())
    {
      return not_found("Report not found");
    }

    const bool deleted =
        repository_.remove_for_run(run_public_id);

    if (!deleted)
    {
      return internal_error("Failed to delete report");
    }

    ReportResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Report deleted";
    result.report = *existing;

    return result;
  }

  bool ReportService::is_valid_summary(
      const std::string &summary) noexcept
  {
    return summary.size() >= MIN_SUMMARY_LENGTH &&
           summary.size() <= MAX_SUMMARY_LENGTH;
  }

  bool ReportService::is_valid_raw_json(
      const std::string &raw_json) noexcept
  {
    return !raw_json.empty() &&
           raw_json.size() <= MAX_RAW_JSON_LENGTH;
  }

  ReportResult ReportService::validation_error(
      std::string message)
  {
    ReportResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  ReportResult ReportService::not_found(
      std::string message)
  {
    ReportResult result;
    result.success = false;
    result.status = 404;
    result.error = "report_not_found";
    result.message = std::move(message);
    return result;
  }

  ReportResult ReportService::internal_error(
      std::string message)
  {
    ReportResult result;
    result.success = false;
    result.status = 500;
    result.error = "internal_error";
    result.message = std::move(message);
    return result;
  }

  ReportListResult ReportService::list_error(
      int status,
      std::string error,
      std::string message)
  {
    ReportListResult result;
    result.success = false;
    result.status = status;
    result.error = std::move(error);
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::reports
