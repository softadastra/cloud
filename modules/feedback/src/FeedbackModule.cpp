#include <feedback/FeedbackModule.hpp>
#include <feedback/controllers/FeedbackController.hpp>
#include <vix.hpp>
namespace cloud::feedback { const char *FeedbackModule::name(){ return "feedback"; } void FeedbackModule::register_routes(vix::App &app){ controllers::FeedbackController::register_routes(app); } }
