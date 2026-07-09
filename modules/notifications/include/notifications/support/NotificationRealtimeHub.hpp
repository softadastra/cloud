#ifndef cloud_notifications_realtime_hub_hpp
#define cloud_notifications_realtime_hub_hpp

#include <cstdint>
#include <functional>
#include <string>

namespace cloud::notifications::support
{
  struct NotificationRealtimeEvent
  {
    std::string id;
    std::string workspace_id;
    std::string project_id;
    std::string recipient_user_id;
    std::string actor_user_id;
    std::string type;
    std::string title;
    std::string message;
    std::string data_json;
    std::int64_t read_at{0};
    std::int64_t created_at{0};
  };

  class NotificationRealtimeHub
  {
  public:
    using Subscriber = std::function<void(const NotificationRealtimeEvent &event)>;

    static void subscribe(Subscriber subscriber);
    static void emit(const NotificationRealtimeEvent &event);
  };
} // namespace cloud::notifications::support

#endif // cloud_notifications_realtime_hub_hpp
