#include <notifications/controllers/NotificationsController.hpp>

#include <auth/middleware/AuthMiddleware.hpp>
#include <notifications/services/NotificationService.hpp>

#include <string>
#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::notifications::controllers
{
  namespace
  {
    using cloud::auth::middleware::AuthContext;
    using cloud::notifications::dto::ListNotificationsRequest;
    using cloud::notifications::dto::NotificationActionRequest;
    using cloud::notifications::dto::PreferencesRequest;
    using cloud::notifications::dto::PreferencesUpdateRequest;
    using cloud::notifications::services::NotificationService;

    void error(vix::Response &res, int status, const std::string &code, const std::string &message)
    {
      res.status(status).json(vix::json::o("ok", false, "error", code, "message", message));
    }

    AuthContext auth_context(vix::Request &req)
    {
      try { return req.state<AuthContext>(); } catch (...) { return {}; }
    }

    std::string body_string(vix::Request &req, const std::string &key)
    {
      if (req.body().empty()) return {};
      try
      {
        const auto &body = req.json();
        return body.is_object() ? body.value(key, "") : std::string{};
      }
      catch (...) { return {}; }
    }

    int body_int(vix::Request &req, const std::string &key, int fallback)
    {
      if (req.body().empty()) return fallback;
      try
      {
        const auto &body = req.json();
        if (!body.is_object() || !body.contains(key)) return fallback;
        return body[key].get<int>();
      }
      catch (...) { return fallback; }
    }

    bool body_bool(vix::Request &req, const std::string &key, bool fallback)
    {
      if (req.body().empty()) return fallback;
      try
      {
        const auto &body = req.json();
        if (!body.is_object() || !body.contains(key)) return fallback;
        return body[key].get<bool>();
      }
      catch (...) { return fallback; }
    }

    void list_notifications(vix::Request &req, vix::Response &res)
    {
      const auto ctx = auth_context(req);
      ListNotificationsRequest request;
      request.user_id = ctx.user_id;
      request.workspace_id = body_string(req, "workspace_id");
      request.project_id = body_string(req, "project_id");
      request.filter = body_string(req, "filter");
      request.limit = body_int(req, "limit", 50);

      NotificationService service;
      auto result = service.list(request);
      if (result.failed()) return error(res, 400, result.code(), result.message());

      vix::json::Json items = vix::json::Json::array();
      for (const auto &item : result.value()) items.push_back(item.to_json());
      res.json(vix::json::o("ok", true, "data", vix::json::o("notifications", items)));
    }

    void unread_count(vix::Request &req, vix::Response &res)
    {
      const auto ctx = auth_context(req);
      NotificationService service;
      auto result = service.unread_count(ctx.user_id);
      if (result.failed()) return error(res, 400, result.code(), result.message());
      res.json(vix::json::o("ok", true, "data", vix::json::o("unread_count", result.value())));
    }

    void mark_read(vix::Request &req, vix::Response &res)
    {
      const auto ctx = auth_context(req);
      NotificationActionRequest request;
      request.user_id = ctx.user_id;
      request.notification_id = body_string(req, "notification_id");
      NotificationService service;
      auto result = service.mark_read(request);
      if (result.failed()) return error(res, 404, result.code(), result.message());
      res.json(vix::json::o("ok", true, "data", result.value().to_json()));
    }

    void mark_all_read(vix::Request &req, vix::Response &res)
    {
      const auto ctx = auth_context(req);
      NotificationService service;
      auto result = service.mark_all_read(ctx.user_id);
      if (result.failed()) return error(res, 400, result.code(), result.message());
      res.json(vix::json::o("ok", true, "data", vix::json::o("unread_count", result.value())));
    }

    void show_preferences(vix::Request &req, vix::Response &res)
    {
      const auto ctx = auth_context(req);
      PreferencesRequest request{ctx.user_id, body_string(req, "workspace_id")};
      NotificationService service;
      auto result = service.show_preferences(request);
      if (result.failed()) return error(res, 400, result.code(), result.message());
      res.json(vix::json::o("ok", true, "data", result.value().to_json()));
    }

    void update_preferences(vix::Request &req, vix::Response &res)
    {
      const auto ctx = auth_context(req);
      PreferencesUpdateRequest request;
      request.user_id = ctx.user_id;
      request.workspace_id = body_string(req, "workspace_id");
      request.notify_invites = body_bool(req, "notify_invites", true);
      request.notify_member_changes = body_bool(req, "notify_member_changes", true);
      request.notify_tokens = body_bool(req, "notify_tokens", true);
      request.notify_projects = body_bool(req, "notify_projects", true);
      request.notify_packages = body_bool(req, "notify_packages", true);
      request.notify_lockfiles = body_bool(req, "notify_lockfiles", true);
      request.notify_build_reports = body_bool(req, "notify_build_reports", true);
      request.notify_security = body_bool(req, "notify_security", true);
      NotificationService service;
      auto result = service.update_preferences(request);
      if (result.failed()) return error(res, 400, result.code(), result.message());
      res.json(vix::json::o("ok", true, "data", result.value().to_json()));
    }
  }

  void NotificationsController::register_routes(vix::App &app)
  {
    app.post("/api/notifications/list", list_notifications);
    app.post("/api/notifications/unread_count", unread_count);
    app.post("/api/notifications/mark_read", mark_read);
    app.post("/api/notifications/mark_all_read", mark_all_read);
    app.post("/api/notifications/preferences/show", show_preferences);
    app.post("/api/notifications/preferences/update", update_preferences);
    app.post("/api/activity/list_workspace", list_notifications);
    app.post("/api/activity/list_project", list_notifications);
  }
}
