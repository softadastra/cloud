#ifndef cloud_feedback_module_hpp
#define cloud_feedback_module_hpp
namespace vix { class App; }
namespace cloud::feedback { class FeedbackModule { public: static const char *name(); static void register_routes(vix::App &app); }; }
#endif
