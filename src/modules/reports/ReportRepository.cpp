/*
 * ReportRepository.cpp
 */

#include "modules/reports/ReportRepository.hpp"

#include <algorithm>
#include <stdexcept>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::modules::reports
{
  ReportRepository::ReportRepository(
      std::shared_ptr<vix::db::Database> database)
      : database_(std::move(database))
  {
    if (database_ == nullptr)
    {
      throw std::invalid_argument(
          "ReportRepository requires a valid database instance");
    }
  }

  bool ReportRepository::create(
      const std::string &public_id,
      std::int64_t run_id,
      const std::string &summary,
      const std::string &raw_json,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "INSERT INTO reports ("
          "public_id, run_id, summary, raw_json, created_at, updated_at"
          ") VALUES (?, ?, ?, ?, ?, ?)",
          public_id,
          run_id,
          summary,
          raw_json,
          now_ms,
          now_ms);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to create report",
          "run_id", run_id,
          "error", e.what());

      return false;
    }
  }

  std::vector<Report> ReportRepository::all(
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "rp.id, rp.public_id, rp.run_id, r.public_id, "
            "rp.summary, rp.raw_json, "
            "rp.created_at, rp.updated_at "
            "FROM reports rp "
            "JOIN runs r ON r.id = rp.run_id "
            "ORDER BY rp.id DESC "
            "LIMIT ? OFFSET ?",
            safe_limit,
            safe_offset);

    std::vector<Report> reports;

    while (rows->next())
    {
      reports.push_back(
          report_from_current_row(rows->row()));
    }

    return reports;
  }

  std::optional<Report> ReportRepository::find_by_id(
      std::int64_t id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "rp.id, rp.public_id, rp.run_id, r.public_id, "
            "rp.summary, rp.raw_json, "
            "rp.created_at, rp.updated_at "
            "FROM reports rp "
            "JOIN runs r ON r.id = rp.run_id "
            "WHERE rp.id = ? "
            "LIMIT 1",
            id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return report_from_current_row(rows->row());
  }

  std::optional<Report> ReportRepository::find_by_public_id(
      const std::string &public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "rp.id, rp.public_id, rp.run_id, r.public_id, "
            "rp.summary, rp.raw_json, "
            "rp.created_at, rp.updated_at "
            "FROM reports rp "
            "JOIN runs r ON r.id = rp.run_id "
            "WHERE rp.public_id = ? "
            "LIMIT 1",
            public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return report_from_current_row(rows->row());
  }

  std::optional<Report> ReportRepository::find_by_run_public_id(
      const std::string &run_public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "rp.id, rp.public_id, rp.run_id, r.public_id, "
            "rp.summary, rp.raw_json, "
            "rp.created_at, rp.updated_at "
            "FROM reports rp "
            "JOIN runs r ON r.id = rp.run_id "
            "WHERE r.public_id = ? "
            "LIMIT 1",
            run_public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return report_from_current_row(rows->row());
  }

  bool ReportRepository::exists_for_run(
      const std::string &run_public_id) const
  {
    auto rows =
        database_->query(
            "SELECT rp.id "
            "FROM reports rp "
            "JOIN runs r ON r.id = rp.run_id "
            "WHERE r.public_id = ? "
            "LIMIT 1",
            run_public_id);

    return rows->next();
  }

  bool ReportRepository::update_for_run(
      const std::string &run_public_id,
      const std::string &summary,
      const std::string &raw_json,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "UPDATE reports "
          "SET summary = ?, raw_json = ?, updated_at = ? "
          "WHERE run_id IN ("
          "SELECT id FROM runs WHERE public_id = ?"
          ")",
          summary,
          raw_json,
          now_ms,
          run_public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to update report",
          "run_id", run_public_id,
          "error", e.what());

      return false;
    }
  }

  bool ReportRepository::remove(
      const std::string &public_id)
  {
    try
    {
      database_->exec(
          "DELETE FROM reports WHERE public_id = ?",
          public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to delete report",
          "report_id", public_id,
          "error", e.what());

      return false;
    }
  }

  bool ReportRepository::remove_for_run(
      const std::string &run_public_id)
  {
    try
    {
      database_->exec(
          "DELETE FROM reports "
          "WHERE run_id IN ("
          "SELECT id FROM runs WHERE public_id = ?"
          ")",
          run_public_id);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to delete report for run",
          "run_id", run_public_id,
          "error", e.what());

      return false;
    }
  }

  std::int64_t ReportRepository::count() const
  {
    auto rows =
        database_->query("SELECT COUNT(*) FROM reports");

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  Report ReportRepository::report_from_current_row(
      const vix::db::ResultRow &row)
  {
    Report report;

    report.id = row.getInt64(0);
    report.public_id = row.getString(1);
    report.run_id = row.getInt64(2);
    report.run_public_id = row.getString(3);
    report.summary = row.getString(4);
    report.raw_json = row.getString(5);
    report.created_at = row.getInt64(6);
    report.updated_at = row.getInt64(7);

    return report;
  }

  int ReportRepository::normalize_limit(int limit) noexcept
  {
    return std::clamp(limit, 1, 100);
  }

  int ReportRepository::normalize_offset(int offset) noexcept
  {
    return std::max(offset, 0);
  }

} // namespace softadastra::cloud::modules::reports
