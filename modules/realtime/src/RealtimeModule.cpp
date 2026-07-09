#include <realtime/RealtimeModule.hpp>

#include <auth/services/AuthService.hpp>
#include <notifications/support/NotificationRealtimeHub.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vix.hpp>
#include <vix/json.hpp>
#include <vix/websocket/AttachedRuntime.hpp>

namespace cloud::realtime
{
  namespace
  {
    std::string user_room(const std::string &user_id)
    {
      return "user:" + user_id;
    }

    void register_notification_bridge(vix::websocket::Server &ws)
    {
      cloud::notifications::support::NotificationRealtimeHub::subscribe(
          [&ws](const cloud::notifications::support::NotificationRealtimeEvent &event)
          {
            if (event.recipient_user_id.empty())
            {
              return;
            }

            ws.broadcast_room_json(
                user_room(event.recipient_user_id),
                "notification.created",
                {
                    "id", event.id,
                    "workspace_id", event.workspace_id,
                    "project_id", event.project_id,
                    "recipient_user_id", event.recipient_user_id,
                    "actor_user_id", event.actor_user_id,
                    "type", event.type,
                    "title", event.title,
                    "message", event.message,
                    "data_json", event.data_json,
                    "read_at", std::to_string(event.read_at),
                    "created_at", std::to_string(event.created_at),
                });
          });
    }

    void register_protocol(vix::websocket::Server &ws)
    {
      auto session_users = std::make_shared<std::unordered_map<void *, std::string>>();

      ws.on_close([session_users, &ws](vix::websocket::Session &session)
                  {
                    const auto key = static_cast<void *>(&session);
                    auto it = session_users->find(key);
                    if (it != session_users->end())
                    {
                      ws.leave_room(session, user_room(it->second));
                      session_users->erase(it);
                    }
                  });

      ws.on_typed_message(
          [session_users, &ws](vix::websocket::Session &session,
                               const std::string &type,
                               const vix::json::kvs &payload)
          {
            if (type == "app.ping")
            {
              session.send_text(vix::websocket::JsonMessage::serialize("app.pong", {"status", "ok", "transport", "websocket"}));
              return;
            }

            if (type != "auth.session")
            {
              session.send_text(vix::websocket::JsonMessage::serialize("auth.required", {"message", "Send auth.session before using realtime."}));
              return;
            }

            const auto session_id = payload.get_string_or("session_id", "");
            if (session_id.empty())
            {
              session.send_text(vix::websocket::JsonMessage::serialize("auth.failed", {"message", "Session id is required."}));
              return;
            }

            cloud::auth::services::AuthService auth;
            auto auth_result = auth.authenticate_session(session_id);
            if (auth_result.failed())
            {
              session.send_text(vix::websocket::JsonMessage::serialize("auth.failed", {"message", "Invalid or expired session."}));
              return;
            }

            const auto user_id = auth_result.value().user_id();
            session_users->insert_or_assign(static_cast<void *>(&session), user_id);
            ws.join_room(session, user_room(user_id));
            session.send_text(vix::websocket::JsonMessage::serialize("auth.ok", {"user_id", user_id}));
          });
    }
  } // namespace

  const char *RealtimeModule::name()
  {
    return "realtime";
  }

  void RealtimeModule::register_routes(vix::App &app)
  {
    app.get("/ws/status", [](vix::Request &req, vix::Response &res)
            {
              (void)req;
              res.json(vix::json::o(
                  "ok", true,
                  "data", vix::json::o(
                      "module", "realtime",
                      "workflow", "attached",
                      "path", "/ws",
                      "port", 9090)));
            });
  }

  int RealtimeModule::run(
      vix::App &app,
      const vix::config::Config &cfg,
      std::shared_ptr<vix::executor::RuntimeExecutor> executor)
  {
    vix::websocket::Server ws{const_cast<vix::config::Config &>(cfg), executor};
    register_protocol(ws);
    register_notification_bridge(ws);
    vix::run_http_and_ws(app, ws, executor, cfg);
    return 0;
  }
} // namespace cloud::realtime
