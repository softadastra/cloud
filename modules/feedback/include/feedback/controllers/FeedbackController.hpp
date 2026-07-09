#ifndef cloud_feedback_controller_hpp
#define cloud_feedback_controller_hpp
namespace vix { class App; }
namespace cloud::feedback::controllers { class FeedbackController { public: static void register_routes(vix::App &app); }; }
#endif
