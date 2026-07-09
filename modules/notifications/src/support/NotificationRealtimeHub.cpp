#include <notifications/support/NotificationRealtimeHub.hpp>

#include <mutex>
#include <vector>

namespace cloud::notifications::support
{
  namespace
  {
    std::mutex &subscribers_mutex()
    {
      static std::mutex mutex;
      return mutex;
    }

    std::vector<NotificationRealtimeHub::Subscriber> &subscribers()
    {
      static std::vector<NotificationRealtimeHub::Subscriber> items;
      return items;
    }
  } // namespace

  void NotificationRealtimeHub::subscribe(Subscriber subscriber)
  {
    std::lock_guard<std::mutex> lock(subscribers_mutex());
    subscribers().push_back(std::move(subscriber));
  }

  void NotificationRealtimeHub::emit(const NotificationRealtimeEvent &event)
  {
    std::vector<Subscriber> snapshot;
    {
      std::lock_guard<std::mutex> lock(subscribers_mutex());
      snapshot = subscribers();
    }

    for (const auto &subscriber : snapshot)
    {
      if (subscriber)
      {
        subscriber(event);
      }
    }
  }
} // namespace cloud::notifications::support
