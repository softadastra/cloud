#ifndef cloud_auth_module_hpp
#define cloud_auth_module_hpp

namespace vix
{
  class App;
}

namespace cloud::auth
{
  class AuthModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::auth

#endif // cloud_auth_module_hpp
