#ifndef cloud_members_module_hpp
#define cloud_members_module_hpp

namespace vix
{
  class App;
}

namespace cloud::members
{
  class MembersModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::members

#endif // cloud_members_module_hpp
