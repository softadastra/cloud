#ifndef cloud_realtime_module_hpp
#define cloud_realtime_module_hpp

#include <memory>

namespace vix { class App; }
namespace vix::config { class Config; }
namespace vix::executor { class RuntimeExecutor; }

namespace cloud::realtime
{
  class RealtimeModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
    static int run(
        vix::App &app,
        const vix::config::Config &cfg,
        std::shared_ptr<vix::executor::RuntimeExecutor> executor);
  };
} // namespace cloud::realtime

#endif // cloud_realtime_module_hpp
