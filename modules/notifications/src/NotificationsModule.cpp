#include <notifications/NotificationsModule.hpp>
#include <notifications/controllers/NotificationsController.hpp>
#include <vix.hpp>
namespace cloud::notifications { const char *NotificationsModule::name(){ return "notifications"; } void NotificationsModule::register_routes(vix::App &app){ controllers::NotificationsController::register_routes(app); } }
