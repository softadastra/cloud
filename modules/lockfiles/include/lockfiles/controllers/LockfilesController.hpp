#ifndef cloud_lockfiles_controller_hpp
#define cloud_lockfiles_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::lockfiles::controllers
{
  class LockfilesController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::lockfiles::controllers

#endif // cloud_lockfiles_controller_hpp
