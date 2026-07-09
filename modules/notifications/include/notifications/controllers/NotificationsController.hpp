#ifndef cloud_notifications_controller_hpp
#define cloud_notifications_controller_hpp
namespace vix { class App; }
namespace cloud::notifications::controllers { class NotificationsController { public: static void register_routes(vix::App &app); }; }
#endif
