/*
 * EventRepository.cpp
 */

#include "modules/events/EventRepository.hpp"

#include <algorithm>
#include <stdexcept>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::modules::events
{
  EventRepository::EventRepository(
      std::shared_ptr<vix::db::Database> database)
      : database_(std::move(database))
  {
    if (database_ == nullptr)
    {
      throw std::invalid_argument(
          "EventRepository requires a valid database instance");
    }
  }

  bool EventRepository::create(
      const std::string &public_id,
      std::int64_t run_id,
      const std::string &event_type,
      const std::string &severity,
      const std::string &message,
      const std::string &payload,
      std::int64_t created_at)
  {
    try
    {
      database_->exec(
          "INSERT INTO run_events ("
          "public_id, run_id, event_type, severity, message, payload, created_at"
          ") VALUES (?, ?, ?, ?, ?, ?, ?)",
          public_id,
          run_id,
          event_type,
          severity,
          message,
          payload,
          created_at);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to create run event",
          "run_id", run_id,
          "event_type", event_type,
          "error", e.what());

      return false;
    }
  }

  std::vector<Event> EventRepository::all(
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "e.id, e.public_id, e.run_id, r.public_id, "
            "e.event_type, e.severity, "
            "COALESCE(e.message, ''), "
            "COALESCE(e.payload, ''), "
            "e.created_at "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "ORDER BY e.id DESC "
            "LIMIT ? OFFSET ?",
            safe_limit,
            safe_offset);

    std::vector<Event> events;

    while (rows->next())
    {
      events.push_back(
          event_from_current_row(rows->row()));
    }

    return events;
  }

  std::vector<Event> EventRepository::all_for_run(
      const std::string &run_public_id,
      int limit,
      int offset) const
  {
    const int safe_limit = normalize_limit(limit);
    const int safe_offset = normalize_offset(offset);

    auto rows =
        database_->query(
            "SELECT "
            "e.id, e.public_id, e.run_id, r.public_id, "
            "e.event_type, e.severity, "
            "COALESCE(e.message, ''), "
            "COALESCE(e.payload, ''), "
            "e.created_at "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "WHERE r.public_id = ? "
            "ORDER BY e.id ASC "
            "LIMIT ? OFFSET ?",
            run_public_id,
            safe_limit,
            safe_offset);

    std::vector<Event> events;

    while (rows->next())
    {
      events.push_back(
          event_from_current_row(rows->row()));
    }

    return events;
  }

  std::optional<Event> EventRepository::find_by_id(
      std::int64_t id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "e.id, e.public_id, e.run_id, r.public_id, "
            "e.event_type, e.severity, "
            "COALESCE(e.message, ''), "
            "COALESCE(e.payload, ''), "
            "e.created_at "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "WHERE e.id = ? "
            "LIMIT 1",
            id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return event_from_current_row(rows->row());
  }

  std::optional<Event> EventRepository::find_by_public_id(
      const std::string &public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "e.id, e.public_id, e.run_id, r.public_id, "
            "e.event_type, e.severity, "
            "COALESCE(e.message, ''), "
            "COALESCE(e.payload, ''), "
            "e.created_at "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "WHERE e.public_id = ? "
            "LIMIT 1",
            public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return event_from_current_row(rows->row());
  }

  std::int64_t EventRepository::count() const
  {
    auto rows =
        database_->query("SELECT COUNT(*) FROM run_events");

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t EventRepository::count_for_run(
      const std::string &run_public_id) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "WHERE r.public_id = ?",
            run_public_id);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t EventRepository::count_by_severity_for_run(
      const std::string &run_public_id,
      const std::string &severity) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "WHERE r.public_id = ? "
            "AND e.severity = ?",
            run_public_id,
            severity);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  std::int64_t EventRepository::count_by_type_for_run(
      const std::string &run_public_id,
      const std::string &event_type) const
  {
    auto rows =
        database_->query(
            "SELECT COUNT(*) "
            "FROM run_events e "
            "JOIN runs r ON r.id = e.run_id "
            "WHERE r.public_id = ? "
            "AND e.event_type = ?",
            run_public_id,
            event_type);

    if (!rows->next())
    {
      return 0;
    }

    return rows->row().getInt64(0);
  }

  bool EventRepository::remove_for_run(
      const std::string &run_public_id)
  {
    try
    {
      database_->exec(
          "DELETE FROM run_events "
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
          "failed to delete run events",
          "run_id", run_public_id,
          "error", e.what());

      return false;
    }
  }

  Event EventRepository::event_from_current_row(
      const vix::db::ResultRow &row)
  {
    Event event;

    event.id = row.getInt64(0);
    event.public_id = row.getString(1);
    event.run_id = row.getInt64(2);
    event.run_public_id = row.getString(3);
    event.event_type = row.getString(4);
    event.severity = row.getString(5);
    event.message = row.getString(6);
    event.payload = row.getString(7);
    event.created_at = row.getInt64(8);

    return event;
  }

  int EventRepository::normalize_limit(int limit) noexcept
  {
    return std::clamp(limit, 1, 500);
  }

  int EventRepository::normalize_offset(int offset) noexcept
  {
    return std::max(offset, 0);
  }

} // namespace softadastra::cloud::modules::events
