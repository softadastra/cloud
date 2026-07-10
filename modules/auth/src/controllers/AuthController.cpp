/**
 *
 *  @file AuthController.cpp
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
#include <auth/controllers/AuthController.hpp>

#include <auth/dto/AuthRequests.hpp>
#include <auth/middleware/AuthMiddleware.hpp>
#include <auth/services/AuthService.hpp>

#include <rix/auth/AuthError.hpp>
#include <auth/support/AuthErrors.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <vix.hpp>
#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::auth::controllers
{
  namespace
  {
    void json_error(
        vix::Response &res,
        int status,
        const std::string &code,
        const std::string &message)
    {
      res.status(status).json(
          vix::json::o(
              "ok", false,
              "error", code,
              "message", message));
    }

    void json_ok(
        vix::Response &res,
        const vix::json::Json &data)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", data));
    }

    void json_message(
        vix::Response &res,
        const std::string &message)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o("message", message)));
    }

    services::AuthService &auth_service()
    {
      static services::AuthService service;
      return service;
    }

    dto::RegisterRequest read_register_request(
        const vix::json::Json &body)
    {
      return {
          body.value("name", ""),
          body.value("email", ""),
          body.value("password", ""),
      };
    }

    dto::LoginRequest read_login_request(
        const vix::json::Json &body)
    {
      return {
          body.value("email", ""),
          body.value("password", ""),
      };
    }

    std::string read_session_id(
        const vix::json::Json &body)
    {
      return body.value("session_id", "");
    }

    dto::UpdateProfileRequest read_update_profile_request(
        const vix::json::Json &body)
    {
      return {
          body.value("session_id", ""),
          body.value("display_name", ""),
          body.value("username", ""),
          body.value("bio", ""),
          body.value("website_url", ""),
          body.value("github_url", ""),
          body.value("public_profile_enabled", false),
      };
    }

    dto::ChangePasswordRequest read_change_password_request(
        const vix::json::Json &body)
    {
      return {
          body.value("session_id", ""),
          body.value("current_password", ""),
          body.value("new_password", ""),
          body.value("confirm_new_password", ""),
      };
    }

    void write_account_error(
        vix::Response &res,
        const rixlib::auth::AuthError &error)
    {
      using rixlib::auth::AuthErrorCode;

      if (error.is(AuthErrorCode::InvalidSession) ||
          error.is(AuthErrorCode::SessionExpired) ||
          error.is(AuthErrorCode::SessionRevoked))
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      if (error.is(AuthErrorCode::InvalidCredentials))
      {
        json_error(res, 403, "current_password_invalid", "Current password is invalid.");
        return;
      }

      if (error.is(AuthErrorCode::InvalidPassword))
      {
        json_error(res, 400, "password_too_weak", error.message().empty() ? "Password is too weak." : error.message());
        return;
      }

      if (error.is(AuthErrorCode::InvalidInput) || error.is(AuthErrorCode::ValidationError))
      {
        if (error.message() == "username_invalid")
        {
          json_error(res, 400, "username_invalid", "Username must be 3 to 32 characters and use only letters, numbers, hyphen or underscore.");
          return;
        }

        if (error.message() == "username_already_taken")
        {
          json_error(res, 409, "username_already_taken", "This username is already taken.");
          return;
        }

        json_error(res, 400, "invalid_request", error.message().empty() ? "Invalid request." : error.message());
        return;
      }

      if (error.is(AuthErrorCode::UserNotFound))
      {
        json_error(res, 404, "user_not_found", "User not found.");
        return;
      }

      json_error(res, 500, "auth_error", error.message().empty() ? "Authentication service error." : error.message());
    }

    bool require_json_object(
        const vix::json::Json &body,
        vix::Response &res)
    {
      if (body.is_object())
      {
        return true;
      }

      json_error(
          res,
          400,
          "invalid_request",
          "Expected JSON object body.");

      return false;
    }

    std::string bearer_session_id(const vix::Request &req)
    {
      auto session_id = req.header("X-Session-Id");

      if (session_id.empty())
      {
        session_id = req.header("x-session-id");
      }

      if (!session_id.empty())
      {
        return session_id;
      }

      auto authorization = req.header("Authorization");

      if (authorization.empty())
      {
        authorization = req.header("authorization");
      }

      const std::string prefix = "Bearer ";

      if (authorization.rfind(prefix, 0) == 0)
      {
        return authorization.substr(prefix.size());
      }

      return {};
    }

    bool safe_path_segment(const std::string &value)
    {
      if (value.empty())
      {
        return false;
      }

      for (const auto ch : value)
      {
        const auto c = static_cast<unsigned char>(ch);

        if (std::isalnum(c) == 0 && ch != '_' && ch != '-' && ch != '.')
        {
          return false;
        }
      }

      return value != "." && value != "..";
    }

    std::string avatar_extension_for_content_type(const std::string &content_type)
    {
      if (content_type.rfind("image/jpeg", 0) == 0)
      {
        return ".jpg";
      }

      if (content_type.rfind("image/png", 0) == 0)
      {
        return ".png";
      }

      if (content_type.rfind("image/webp", 0) == 0)
      {
        return ".webp";
      }

      return {};
    }

    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_public_id(const std::string &prefix, std::int64_t suffix = 0)
    {
      std::ostringstream out;
      out << prefix << "_" << now_timestamp() << "_" << suffix;
      return out.str();
    }

    std::string utc_date(std::int64_t timestamp)
    {
      std::time_t raw = static_cast<std::time_t>(timestamp);
      std::tm tm{};
#ifdef _WIN32
      gmtime_s(&tm, &raw);
#else
      gmtime_r(&raw, &tm);
#endif
      char buffer[16]{};
      std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
      return buffer;
    }

    int utc_year(std::int64_t timestamp)
    {
      std::time_t raw = static_cast<std::time_t>(timestamp);
      std::tm tm{};
#ifdef _WIN32
      gmtime_s(&tm, &raw);
#else
      gmtime_r(&raw, &tm);
#endif
      return tm.tm_year + 1900;
    }

    bool leap_year(int year)
    {
      return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
    }

    std::int64_t utc_year_start(int year)
    {
      std::tm tm{};
      tm.tm_year = year - 1900;
      tm.tm_mon = 0;
      tm.tm_mday = 1;
#ifdef _WIN32
      return static_cast<std::int64_t>(_mkgmtime(&tm));
#else
      return static_cast<std::int64_t>(timegm(&tm));
#endif
    }

    int contribution_level(std::int64_t count)
    {
      if (count <= 0)
        return 0;
      if (count == 1)
        return 1;
      if (count <= 3)
        return 2;
      if (count <= 6)
        return 3;
      return 4;
    }

    void ensure_public_profile_pins_table(vix::db::Database &db)
    {
      db.exec(
          "CREATE TABLE IF NOT EXISTS public_profile_pins ("
          "id TEXT PRIMARY KEY, "
          "user_id TEXT NOT NULL, "
          "package_id TEXT NOT NULL, "
          "sort_order INTEGER NOT NULL DEFAULT 0, "
          "created_at INTEGER NOT NULL, "
          "updated_at INTEGER NOT NULL, "
          "UNIQUE(user_id, package_id))");
    }

    vix::json::Json public_package_json(
        vix::db::Database &db,
        const vix::db::ResultRow &row)
    {
      auto version_rows = db.query(
          "SELECT COUNT(*), COALESCE((SELECT version FROM package_versions WHERE package_id = ? ORDER BY created_at DESC LIMIT 1), '') "
          "FROM package_versions WHERE package_id = ?",
          row.getString(0),
          row.getString(0));

      std::int64_t versions_count = 0;
      std::string latest_version;
      if (version_rows->next())
      {
        versions_count = version_rows->row().getInt64(0);
        latest_version = version_rows->row().getString(1);
      }

      auto item = vix::json::Json::object();
      item["id"] = row.getString(0);
      item["name"] = row.getString(1);
      item["description"] = row.getString(2);
      item["repository_url"] = row.getString(3);
      item["visibility"] = row.getString(4);
      item["created_at"] = row.getInt64(6);
      item["updated_at"] = row.getInt64(7);
      item["latest_version"] = latest_version;
      item["versions_count"] = versions_count;
      item["package_type"] = "Vix package";
      return item;
    }

    bool looks_like_email(const std::string &value)
    {
      const auto at = value.find('@');

      if (at == std::string::npos ||
          at == 0 ||
          at + 1 >= value.size())
      {
        return false;
      }

      const auto dot = value.find('.', at + 1);

      return dot != std::string::npos &&
             dot > at + 1 &&
             dot + 1 < value.size();
    }

    bool is_public_profile_visible(
        bool enabled,
        const std::string &username)
    {
      return enabled &&
             !username.empty() &&
             !looks_like_email(username);
    }

    bool is_public_package_visible(const std::string &visibility, bool active)
    {
      return active && visibility == "public";
    }

    bool can_expose_public_activity(const std::string &visibility)
    {
      return visibility == "public";
    }

    void ensure_public_activity_table(vix::db::Database &db)
    {
      db.exec(
          "CREATE TABLE IF NOT EXISTS public_activity_events ("
          "id TEXT PRIMARY KEY, "
          "user_id TEXT NOT NULL, "
          "workspace_id TEXT, "
          "project_id TEXT, "
          "package_id TEXT, "
          "type TEXT NOT NULL, "
          "title TEXT NOT NULL, "
          "data_json TEXT, "
          "visibility TEXT NOT NULL DEFAULT 'public', "
          "created_at INTEGER NOT NULL)");
    }


    void ensure_founding_supporters_table(vix::db::Database &db)
    {
      db.exec(
          "CREATE TABLE IF NOT EXISTS founding_supporters ("
          "id TEXT PRIMARY KEY, "
          "user_id TEXT NULL, "
          "tier TEXT NOT NULL, "
          "status TEXT NOT NULL DEFAULT 'active', "
          "display_name TEXT NOT NULL, "
          "username TEXT NULL, "
          "project_name TEXT NULL, "
          "website_url TEXT NULL, "
          "github_url TEXT NULL, "
          "public_visible INTEGER NOT NULL DEFAULT 1, "
          "stronger_visibility INTEGER NOT NULL DEFAULT 0, "
          "notes TEXT NULL, "
          "started_at INTEGER NOT NULL, "
          "expires_at INTEGER NULL, "
          "created_at INTEGER NOT NULL, "
          "updated_at INTEGER NOT NULL)");
      db.exec("CREATE INDEX IF NOT EXISTS idx_founding_supporters_user_id ON founding_supporters(user_id)");
      db.exec("CREATE INDEX IF NOT EXISTS idx_founding_supporters_public ON founding_supporters(status, public_visible, tier)");
    }

    std::string env_value(const std::string &name)
    {
      if (const char *value = std::getenv(name.c_str()))
      {
        return value;
      }

      std::ifstream input{".env"};
      std::string line;
      const auto prefix = name + "=";
      while (std::getline(input, line))
      {
        if (line.rfind(prefix, 0) == 0)
        {
          return line.substr(prefix.size());
        }
      }
      return {};
    }

    vix::json::Json supporter_public_json(const vix::db::ResultRow &row)
    {
      return vix::json::o(
          "tier", row.getString(0),
          "display_name", row.getString(1),
          "username", row.getString(2),
          "project_name", row.getString(3),
          "website_url", row.getString(4),
          "github_url", row.getString(5),
          "stronger_visibility", row.getInt64(6) != 0,
          "started_at", row.getInt64(7));
    }

    vix::json::Json active_public_supporter_for_user(vix::db::Database &db, const std::string &user_id)
    {
      auto rows = db.query(
          "SELECT tier, display_name, COALESCE(username, ''), COALESCE(project_name, ''), "
          "COALESCE(website_url, ''), COALESCE(github_url, ''), stronger_visibility, started_at "
          "FROM founding_supporters "
          "WHERE user_id = ? AND status = 'active' AND public_visible = 1 "
          "ORDER BY stronger_visibility DESC, started_at ASC LIMIT 1",
          user_id);

      if (rows->next())
      {
        return supporter_public_json(rows->row());
      }

      return vix::json::Json{};
    }

    void public_profile_show(vix::Request &req, vix::Response &res)
    {
      auto username = req.has_query("username") ? req.query_value("username") : std::string{};
      auto selected_year = utc_year(now_timestamp());

      if (req.has_query("year"))
      {
        try
        {
          selected_year = std::stoi(req.query_value("year"));
        }
        catch (...)
        {
          json_error(res, 400, "contribution_year_invalid", "Contribution year is invalid.");
          return;
        }
      }

      if (username.empty() && !req.body().empty())
      {
        try
        {
          const auto &body = req.json();
          if (body.is_object())
          {
            username = body.value("username", "");
            selected_year = body.value("year", selected_year);
          }
        }
        catch (...)
        {
        }
      }

      if (username.empty())
      {
        json_error(res, 400, "username_required", "Username is required.");
        return;
      }

      const auto current_year = utc_year(now_timestamp());
      if (selected_year < 1970 || selected_year > current_year)
      {
        json_error(res, 400, "contribution_year_invalid", "Contribution year is invalid.");
        return;
      }

      try
      {
        vix::config::Config cfg{".env"};
        vix::db::Database db{cfg};
        ensure_public_activity_table(db);
        ensure_public_profile_pins_table(db);
        ensure_founding_supporters_table(db);

        auto profile_rows = db.query(
            "SELECT user_id, COALESCE(display_name, ''), COALESCE(username, ''), COALESCE(bio, ''), "
            "COALESCE(avatar_url, ''), COALESCE(website_url, ''), COALESCE(github_url, ''), public_profile_enabled "
            "FROM user_profiles WHERE username = ? LIMIT 1",
            username);

        if (!profile_rows->next())
        {
          json_error(res, 404, "profile_not_found", "Profile was not found.");
          return;
        }

        const auto &profile_row = profile_rows->row();
        const auto user_id = profile_row.getString(0);
        const auto stored_display_name = profile_row.getString(1);
        const auto profile_username = profile_row.getString(2);
        const auto profile_enabled = profile_row.getInt64(7) != 0;

        const auto public_display_name =
            looks_like_email(stored_display_name)
                ? profile_username
                : stored_display_name;

        if (!is_public_profile_visible(profile_enabled, profile_username))
        {
          json_error(res, 404, "profile_not_found", "Profile was not found.");
          return;
        }

        const auto supporter_badge = active_public_supporter_for_user(db, user_id);

        auto pinned_packages = vix::json::Json::array();
        auto pinned_rows = db.query(
            "SELECT p.id, p.name, COALESCE(p.description, ''), COALESCE(p.repository_url, ''), p.visibility, p.active, p.created_at, p.updated_at "
            "FROM public_profile_pins pin "
            "JOIN packages p ON p.id = pin.package_id "
            "WHERE pin.user_id = ? AND p.owner_user_id = ? AND p.visibility = 'public' AND p.active = 1 "
            "ORDER BY pin.sort_order ASC, pin.created_at ASC LIMIT 6",
            user_id,
            user_id);

        while (pinned_rows->next())
        {
          const auto &row = pinned_rows->row();
          if (is_public_package_visible(row.getString(4), row.getInt64(5) != 0))
          {
            pinned_packages.push_back(public_package_json(db, row));
          }
        }

        auto packages = vix::json::Json::array();
        auto package_rows = db.query(
            "SELECT id, name, COALESCE(description, ''), COALESCE(repository_url, ''), visibility, active, created_at, updated_at "
            "FROM packages WHERE owner_user_id = ? AND visibility = 'public' AND active = 1 ORDER BY updated_at DESC LIMIT 50",
            user_id);

        while (package_rows->next())
        {
          const auto &row = package_rows->row();
          if (is_public_package_visible(row.getString(4), row.getInt64(5) != 0))
          {
            packages.push_back(public_package_json(db, row));
          }
        }

        auto recent_activity = vix::json::Json::array();
        auto activity_rows = db.query(
            "SELECT id, COALESCE(package_id, ''), type, title, COALESCE(data_json, '{}'), visibility, created_at "
            "FROM public_activity_events WHERE user_id = ? AND visibility = 'public' ORDER BY created_at DESC LIMIT 30",
            user_id);

        std::int64_t recent_count = 0;
        while (activity_rows->next())
        {
          const auto &row = activity_rows->row();
          if (!can_expose_public_activity(row.getString(5)))
          {
            continue;
          }

          recent_count += 1;
          auto activity_json = vix::json::Json::object();
          activity_json["id"] = row.getString(0);
          activity_json["package_id"] = row.getString(1);
          activity_json["type"] = row.getString(2);
          activity_json["title"] = row.getString(3);
          activity_json["data_json"] = row.getString(4);
          activity_json["created_at"] = row.getInt64(6);
          recent_activity.push_back(activity_json);
        }

        auto count_rows = db.query(
            "SELECT COUNT(*) FROM public_activity_events WHERE user_id = ? AND visibility = 'public'",
            user_id);
        const auto total_contributions = count_rows->next() ? count_rows->row().getInt64(0) : recent_count;

        auto contribution_years = vix::json::Json::array();
        auto year_rows = db.query(
            "SELECT DISTINCT CAST(strftime('%Y', created_at, 'unixepoch') AS INTEGER) AS year "
            "FROM public_activity_events WHERE user_id = ? AND visibility = 'public' "
            "ORDER BY year DESC",
            user_id);

        bool selected_year_exists = false;
        bool current_year_exists = false;
        while (year_rows->next())
        {
          const auto year = static_cast<int>(year_rows->row().getInt64(0));
          contribution_years.push_back(year);
          selected_year_exists = selected_year_exists || year == selected_year;
          current_year_exists = current_year_exists || year == current_year;
        }

        if (contribution_years.empty())
        {
          contribution_years.push_back(current_year);
        }
        else if (!selected_year_exists && selected_year == current_year && !current_year_exists)
        {
          auto years_with_current = vix::json::Json::array();
          years_with_current.push_back(current_year);
          for (const auto &year : contribution_years)
          {
            years_with_current.push_back(year);
          }
          contribution_years = years_with_current;
        }

        std::unordered_map<std::string, std::int64_t> counts_by_date;
        const auto year_start = utc_year_start(selected_year);
        const auto next_year_start = utc_year_start(selected_year + 1);
        auto grid_rows = db.query(
            "SELECT strftime('%Y-%m-%d', created_at, 'unixepoch') AS day, COUNT(*) "
            "FROM public_activity_events "
            "WHERE user_id = ? AND visibility = 'public' AND created_at >= ? AND created_at < ? "
            "GROUP BY day",
            user_id,
            year_start,
            next_year_start);

        while (grid_rows->next())
        {
          counts_by_date[grid_rows->row().getString(0)] = grid_rows->row().getInt64(1);
        }

        auto contribution_grid = vix::json::Json::array();
        const auto number_of_days = leap_year(selected_year) ? 366 : 365;
        for (int day = 0; day < number_of_days; ++day)
        {
          const auto timestamp = year_start + static_cast<std::int64_t>(day) * 86400;
          const auto date = utc_date(timestamp);
          const auto item = counts_by_date.find(date);
          const auto count = item == counts_by_date.end() ? static_cast<std::int64_t>(0) : item->second;

          auto cell = vix::json::Json::object();
          cell["date"] = date;
          cell["count"] = count;
          cell["level"] = contribution_level(count);
          contribution_grid.push_back(cell);
        }

        json_ok(res, vix::json::o(
                         "profile", vix::json::o("display_name", public_display_name, "username", profile_username, "bio", profile_row.getString(3), "avatar_url", profile_row.getString(4), "website_url", profile_row.getString(5), "github_url", profile_row.getString(6), "public_profile_enabled", profile_enabled),
                         "pinned_packages", pinned_packages,
                         "public_packages", packages,
                         "contribution_grid", contribution_grid,
                         "contribution_years", contribution_years,
                         "selected_contribution_year", selected_year,
                         "recent_activity", recent_activity,
                         "public_activity", recent_activity,
                         "stats", vix::json::o("public_packages_count", static_cast<std::int64_t>(packages.size()), "public_contributions_count", total_contributions, "pinned_packages_count", static_cast<std::int64_t>(pinned_packages.size()))));
      }
      catch (...)
      {
        json_error(res, 500, "public_profile_error", "Could not load public profile.");
      }
    }

    std::string current_user_id(vix::Request &req)
    {
      try
      {
        return req.state<cloud::auth::middleware::AuthContext>().user_id;
      }
      catch (...)
      {
        return {};
      }
    }


    void public_support_config(vix::Request &, vix::Response &res)
    {
      json_ok(res, vix::json::o(
          "supporter_payment_url", env_value("SOFTADASTRA_SUPPORTER_PAYMENT_URL"),
          "builder_payment_url", env_value("SOFTADASTRA_BUILDER_PAYMENT_URL"),
          "contact_email", env_value("SOFTADASTRA_SUPPORT_CONTACT_EMAIL"),
          "contact_url", env_value("SOFTADASTRA_SUPPORT_CONTACT_URL")));
    }

    void public_supporters_list(vix::Request &, vix::Response &res)
    {
      try
      {
        vix::config::Config cfg{".env"};
        vix::db::Database db{cfg};
        ensure_founding_supporters_table(db);

        auto supporters = vix::json::Json::array();
        auto rows = db.query(
            "SELECT tier, display_name, COALESCE(username, ''), COALESCE(project_name, ''), "
            "COALESCE(website_url, ''), COALESCE(github_url, ''), stronger_visibility, started_at "
            "FROM founding_supporters "
            "WHERE status = 'active' AND public_visible = 1 "
            "ORDER BY stronger_visibility DESC, started_at ASC, display_name ASC");

        while (rows->next())
        {
          supporters.push_back(supporter_public_json(rows->row()));
        }

        json_ok(res, vix::json::o("supporters", supporters));
      }
      catch (...)
      {
        json_error(res, 500, "supporters_error", "Could not load founding supporters.");
      }
    }

    void supporter_me(vix::Request &req, vix::Response &res)
    {
      const auto user_id = current_user_id(req);
      if (user_id.empty())
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      try
      {
        vix::config::Config cfg{".env"};
        vix::db::Database db{cfg};
        ensure_founding_supporters_table(db);

        auto rows = db.query(
            "SELECT tier, status, display_name, started_at, public_visible, stronger_visibility "
            "FROM founding_supporters WHERE user_id = ? "
            "ORDER BY status = 'active' DESC, stronger_visibility DESC, started_at ASC LIMIT 1",
            user_id);

        if (!rows->next())
        {
          json_ok(res, vix::json::o("supporter", vix::json::Json{}));
          return;
        }

        const auto &row = rows->row();
        json_ok(res, vix::json::o(
            "supporter", vix::json::o(
                "tier", row.getString(0),
                "status", row.getString(1),
                "display_name", row.getString(2),
                "started_at", row.getInt64(3),
                "public_visible", row.getInt64(4) != 0,
                "stronger_visibility", row.getInt64(5) != 0)));
      }
      catch (...)
      {
        json_error(res, 500, "supporter_status_error", "Could not load supporter status.");
      }
    }

    void profile_pins_list(vix::Request &req, vix::Response &res)
    {
      const auto user_id = current_user_id(req);
      if (user_id.empty())
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      try
      {
        vix::config::Config cfg{".env"};
        vix::db::Database db{cfg};
        ensure_public_profile_pins_table(db);

        auto pins = vix::json::Json::array();
        auto ids = vix::json::Json::array();
        auto rows = db.query(
            "SELECT p.id, p.name, COALESCE(p.description, ''), COALESCE(p.repository_url, ''), p.visibility, p.active, p.created_at, p.updated_at "
            "FROM public_profile_pins pin JOIN packages p ON p.id = pin.package_id "
            "WHERE pin.user_id = ? AND p.owner_user_id = ? AND p.visibility = 'public' AND p.active = 1 "
            "ORDER BY pin.sort_order ASC, pin.created_at ASC",
            user_id,
            user_id);

        while (rows->next())
        {
          const auto &row = rows->row();
          ids.push_back(row.getString(0));
          pins.push_back(public_package_json(db, row));
        }

        json_ok(res, vix::json::o("package_ids", ids, "pinned_packages", pins));
      }
      catch (...)
      {
        json_error(res, 500, "profile_pins_error", "Could not load profile pins.");
      }
    }

    bool validate_public_pin_package(
        vix::db::Database &db,
        const std::string &user_id,
        const std::string &package_id,
        vix::Response &res)
    {
      auto rows = db.query(
          "SELECT owner_user_id, visibility, active FROM packages WHERE id = ? LIMIT 1",
          package_id);

      if (!rows->next())
      {
        json_error(res, 404, "package_not_found", "Package was not found.");
        return false;
      }

      const auto &row = rows->row();
      if (row.getString(0) != user_id)
      {
        json_error(res, 403, "package_not_owned_by_user", "You can only pin your own packages.");
        return false;
      }

      if (row.getString(1) != "public")
      {
        json_error(res, 400, "package_not_public", "Only public packages can be pinned.");
        return false;
      }

      if (row.getInt64(2) == 0)
      {
        json_error(res, 404, "package_not_found", "Package was not found.");
        return false;
      }

      return true;
    }

    void profile_pins_update(vix::Request &req, vix::Response &res)
    {
      const auto user_id = current_user_id(req);
      if (user_id.empty())
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      const auto &body = req.json();
      if (!body.is_object() || !body.contains("package_ids") || !body["package_ids"].is_array())
      {
        json_error(res, 400, "invalid_request", "package_ids must be an array.");
        return;
      }

      std::vector<std::string> package_ids;
      for (const auto &item : body["package_ids"])
      {
        if (!item.is_string())
        {
          json_error(res, 400, "invalid_request", "package_ids must contain strings.");
          return;
        }
        const auto id = item.get<std::string>();
        if (std::find(package_ids.begin(), package_ids.end(), id) == package_ids.end())
        {
          package_ids.push_back(id);
        }
      }

      if (package_ids.size() > 6)
      {
        json_error(res, 400, "too_many_pins", "You can pin up to 6 public packages.");
        return;
      }

      try
      {
        vix::config::Config cfg{".env"};
        vix::db::Database db{cfg};
        ensure_public_profile_pins_table(db);

        for (const auto &package_id : package_ids)
        {
          if (!validate_public_pin_package(db, user_id, package_id, res))
          {
            return;
          }
        }

        db.exec("DELETE FROM public_profile_pins WHERE user_id = ?", user_id);

        const auto now = now_timestamp();
        for (std::size_t index = 0; index < package_ids.size(); ++index)
        {
          db.exec(
              "INSERT INTO public_profile_pins (id, user_id, package_id, sort_order, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?)",
              make_public_id("pin", static_cast<std::int64_t>(index)),
              user_id,
              package_ids[index],
              static_cast<std::int64_t>(index),
              now,
              now);
        }

        profile_pins_list(req, res);
      }
      catch (...)
      {
        json_error(res, 500, "profile_pins_error", "Could not update profile pins.");
      }
    }

    std::string content_type_for_avatar_filename(const std::string &filename)
    {
      if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".jpg")
      {
        return "image/jpeg";
      }

      if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".png")
      {
        return "image/png";
      }

      if (filename.size() >= 5 && filename.substr(filename.size() - 5) == ".webp")
      {
        return "image/webp";
      }

      return "application/octet-stream";
    }
  } // namespace

  void AuthController::register_routes(vix::App &app)
  {
    app.get("/api/auth", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Auth module is available"); });

    app.post("/api/auth/register", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_register_request(body);
      auto registered = auth_service().register_user(request);

      if (registered.failed())
      {
        support::write_auth_error(res, registered.error());
        return;
      }

      const auto &user = registered.value();
      auto profile = auth_service().user_profile(user.id());

      if (profile.failed())
      {
        write_account_error(res, profile.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "user", profile.value().to_json()))); });

    app.post("/api/auth/login", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_login_request(body);
      auto login = auth_service().login(request);

      if (login.failed())
      {
        support::write_auth_error(res, login.error());
        return;
      }

      const auto &result = login.value();
      auto profile = auth_service().user_profile(result.user.id());

      if (profile.failed())
      {
        write_account_error(res, profile.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "user", profile.value().to_json(),
              "session", vix::json::o(
                  "id", result.session.id(),
                  "expires_at", result.session.expires_at()),
              "token", vix::json::o(
                  "value", result.token.value(),
                  "issuer", result.token.issuer(),
                  "expires_at", result.token.expires_at()))); });

    app.post("/api/auth/logout", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto session_id = read_session_id(body);
      auto logout = auth_service().logout(session_id);

      if (logout.failed())
      {
        support::write_auth_error(res, logout.error());
        return;
      }

      json_message(
          res,
          "Logged out successfully."); });

    app.post("/api/auth/me", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto session_id = read_session_id(body);
      auto session = auth_service().authenticate_session(session_id);

      if (session.failed())
      {
        support::write_auth_error(res, session.error());
        return;
      }

      auto profile = auth_service().user_profile(session.value().user_id());

      if (profile.failed())
      {
        write_account_error(res, profile.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "user", profile.value().to_json(),
              "session", vix::json::o(
                  "id", session.value().id(),
                  "user_id", session.value().user_id(),
                  "expires_at", session.value().expires_at(),
                  "last_seen_at", session.value().last_seen_at(),
                  "revoked", session.value().revoked()))); });

    app.post("/api/auth/update_profile", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto updated = auth_service().update_profile(read_update_profile_request(body));

      if (updated.failed())
      {
        write_account_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o("user", updated.value().to_json())); });

    app.post("/api/auth/avatar/upload", [](vix::Request &req, vix::Response &res)
             {
      const auto session_id = bearer_session_id(req);

      if (session_id.empty())
      {
        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      auto session = auth_service().authenticate_session(session_id);

      if (session.failed())
      {
        write_account_error(res, session.error());
        return;
      }

      auto content_type = req.header("Content-Type");

      if (content_type.empty())
      {
        content_type = req.header("content-type");
      }

      const auto extension = avatar_extension_for_content_type(content_type);

      if (extension.empty())
      {
        json_error(res, 400, "invalid_content_type", "Avatar must be a JPG, PNG or WebP image.");
        return;
      }

      const auto &body = req.body();
      constexpr std::size_t max_avatar_size = 2u * 1024u * 1024u;

      if (body.size() > max_avatar_size)
      {
        json_error(res, 413, "avatar_too_large", "Avatar image must be 2MB or smaller.");
        return;
      }

      if (body.empty())
      {
        json_error(res, 400, "avatar_upload_failed", "Avatar image body is required.");
        return;
      }

      const auto user_id = session.value().user_id();
      const std::filesystem::path storage_root{"storage/users"};
      const auto avatar_dir = storage_root / user_id;
      const auto filename = std::string{"avatar"} + extension;
      const auto avatar_path = avatar_dir / filename;

      try
      {
        std::filesystem::create_directories(avatar_dir);

        for (const auto &entry : std::filesystem::directory_iterator(avatar_dir))
        {
          if (entry.is_regular_file() && entry.path().stem() == "avatar")
          {
            std::filesystem::remove(entry.path());
          }
        }

        std::ofstream out(avatar_path, std::ios::binary | std::ios::trunc);

        if (!out.good())
        {
          json_error(res, 500, "avatar_upload_failed", "Could not write avatar image.");
          return;
        }

        out.write(body.data(), static_cast<std::streamsize>(body.size()));
      }
      catch (...)
      {
        json_error(res, 500, "avatar_upload_failed", "Could not write avatar image.");
        return;
      }

      const auto avatar_url = std::string{"/storage/users/"} + user_id + "/" + filename;
      auto updated = auth_service().update_avatar(session_id, avatar_url, avatar_path.string());

      if (updated.failed())
      {
        write_account_error(res, updated.error());
        return;
      }

      json_ok(res, vix::json::o(
          "avatar_url", avatar_url,
          "user", updated.value().to_json())); });

    app.post("/api/auth/avatar/delete", [](vix::Request &req, vix::Response &res)
             {
      const auto session_id = bearer_session_id(req);

      if (session_id.empty())
      {
        const auto &body = req.json();

        if (body.is_object())
        {
          auto deleted = auth_service().delete_avatar(body.value("session_id", ""));

          if (deleted.failed())
          {
            write_account_error(res, deleted.error());
            return;
          }

          if (!deleted.value().empty())
          {
            try { std::filesystem::remove(deleted.value()); } catch (...) {}
          }

          json_ok(res, vix::json::o("message", "Avatar removed."));
          return;
        }

        json_error(res, 401, "unauthenticated", "Authentication is required.");
        return;
      }

      auto deleted = auth_service().delete_avatar(session_id);

      if (deleted.failed())
      {
        write_account_error(res, deleted.error());
        return;
      }

      if (!deleted.value().empty())
      {
        try { std::filesystem::remove(deleted.value()); } catch (...) {}
      }

      json_ok(res, vix::json::o("message", "Avatar removed.")); });

    app.post("/api/profile/pins/list", [](vix::Request &req, vix::Response &res)
             { profile_pins_list(req, res); });

    app.post("/api/profile/pins/update", [](vix::Request &req, vix::Response &res)
             { profile_pins_update(req, res); });

    app.post("/api/public/support/config", [](vix::Request &req, vix::Response &res)
             { public_support_config(req, res); });

    app.post("/api/public/supporters/list", [](vix::Request &req, vix::Response &res)
             { public_supporters_list(req, res); });

    app.post("/api/supporters/me", [](vix::Request &req, vix::Response &res)
             { supporter_me(req, res); });

    app.post("/api/public/users/show", [](vix::Request &req, vix::Response &res)
             { public_profile_show(req, res); });

    app.get("/api/public/users/show", [](vix::Request &req, vix::Response &res)
            { public_profile_show(req, res); });

    app.get("/storage/users/{user_id}/{filename}", [](vix::Request &req, vix::Response &res)
            {
      const auto user_id = req.param("user_id");
      const auto filename = req.param("filename");

      if (!safe_path_segment(user_id) || !safe_path_segment(filename) || filename.rfind("avatar.", 0) != 0)
      {
        json_error(res, 404, "avatar_not_found", "Avatar was not found.");
        return;
      }

      const auto path = std::filesystem::path{"storage/users"} / user_id / filename;

      if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
      {
        json_error(res, 404, "avatar_not_found", "Avatar was not found.");
        return;
      }

      std::ifstream in(path, std::ios::binary);

      if (!in.good())
      {
        json_error(res, 404, "avatar_not_found", "Avatar was not found.");
        return;
      }

      std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      res.header("Content-Type", content_type_for_avatar_filename(filename));
      res.header("Cache-Control", "public, max-age=300");
      res.res.set_body(std::move(body)); });

    app.post("/api/auth/change_password", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      auto changed = auth_service().change_password(read_change_password_request(body));

      if (changed.failed())
      {
        write_account_error(res, changed.error());
        return;
      }

      json_ok(res, vix::json::o("message", "Password changed successfully.")); });
  }
} // namespace cloud::auth::controllers
