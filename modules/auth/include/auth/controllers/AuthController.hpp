#ifndef cloud_auth_controller_hpp
#define cloud_auth_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::auth::controllers
{
  class AuthController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::auth::controllers

#endif // cloud_auth_controller_hpp
