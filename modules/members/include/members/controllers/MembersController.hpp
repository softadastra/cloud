#ifndef cloud_members_controller_hpp
#define cloud_members_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::members::controllers
{
  class MembersController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::members::controllers

#endif // cloud_members_controller_hpp
