#ifndef cloud_notifications_service_hpp
#define cloud_notifications_service_hpp
#include <notifications/dto/NotificationDtos.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace cloud::notifications::services {
template <typename T> class NotificationResult { public: static NotificationResult success(T v){ NotificationResult r; r.ok_=true; r.value_=std::move(v); return r;} static NotificationResult failure(std::string c,std::string m){ NotificationResult r; r.ok_=false; r.code_=std::move(c); r.message_=std::move(m); return r;} bool ok() const{return ok_;} bool failed() const{return !ok_;} const T& value() const{return value_;} const std::string& code() const{return code_;} const std::string& message() const{return message_;} private: bool ok_{false}; T value_{}; std::string code_{"notification_error"}, message_{"Notification operation failed."}; };
class NotificationService { public: NotificationService(); ~NotificationService(); NotificationService(const NotificationService&)=delete; NotificationService& operator=(const NotificationService&)=delete; NotificationResult<dto::NotificationResponse> create(const dto::CreateNotificationRequest &request); NotificationResult<std::vector<dto::NotificationResponse>> list(const dto::ListNotificationsRequest &request) const; NotificationResult<int> unread_count(const std::string &user_id) const; NotificationResult<dto::NotificationResponse> mark_read(const dto::NotificationActionRequest &request); NotificationResult<int> mark_all_read(const std::string &user_id); NotificationResult<dto::PreferencesResponse> show_preferences(const dto::PreferencesRequest &request); NotificationResult<dto::PreferencesResponse> update_preferences(const dto::PreferencesUpdateRequest &request); NotificationResult<int> create_for_workspace_members(const dto::CreateNotificationRequest &request); NotificationResult<int> create_for_project_members(const dto::CreateNotificationRequest &request); private: class Impl; std::unique_ptr<Impl> impl_; };
}
#endif
