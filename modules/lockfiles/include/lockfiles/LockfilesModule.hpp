#ifndef cloud_lockfiles_module_hpp
#define cloud_lockfiles_module_hpp

namespace vix
{
  class App;
}

namespace cloud::lockfiles
{
  class LockfilesModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::lockfiles

#endif // cloud_lockfiles_module_hpp
