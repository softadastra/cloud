#ifndef cloud_notifications_dto_hpp
#define cloud_notifications_dto_hpp
#include <cstdint>
#include <string>
#include <vix/json.hpp>
namespace cloud::notifications::dto {
struct NotificationResponse {
  std::string id, workspace_id, project_id, recipient_user_id, actor_user_id, type, title, message, data_json;
  std::int64_t read_at{0}, created_at{0};
  vix::json::Json to_json() const { return vix::json::o("id", id, "workspace_id", workspace_id, "project_id", project_id, "recipient_user_id", recipient_user_id, "actor_user_id", actor_user_id, "type", type, "title", title, "message", message, "data_json", data_json, "read_at", read_at, "created_at", created_at); }
};
struct CreateNotificationRequest { std::string workspace_id, project_id, recipient_user_id, actor_user_id, type, title, message, data_json; };
struct ListNotificationsRequest { std::string user_id, workspace_id, project_id, filter; int limit{50}; };
struct NotificationActionRequest { std::string user_id, notification_id; };
struct PreferencesRequest { std::string user_id, workspace_id; };
struct PreferencesUpdateRequest { std::string user_id, workspace_id; bool notify_invites{true}, notify_member_changes{true}, notify_tokens{true}, notify_projects{true}, notify_packages{true}, notify_lockfiles{true}, notify_build_reports{true}, notify_security{true}; };
struct PreferencesResponse {
  std::string id, user_id, workspace_id; bool notify_invites{true}, notify_member_changes{true}, notify_tokens{true}, notify_projects{true}, notify_packages{true}, notify_lockfiles{true}, notify_build_reports{true}, notify_security{true}; std::int64_t created_at{0}, updated_at{0};
  vix::json::Json to_json() const { return vix::json::o("id", id, "user_id", user_id, "workspace_id", workspace_id, "notify_invites", notify_invites, "notify_member_changes", notify_member_changes, "notify_tokens", notify_tokens, "notify_projects", notify_projects, "notify_packages", notify_packages, "notify_lockfiles", notify_lockfiles, "notify_build_reports", notify_build_reports, "notify_security", notify_security, "created_at", created_at, "updated_at", updated_at); }
};
}
#endif
