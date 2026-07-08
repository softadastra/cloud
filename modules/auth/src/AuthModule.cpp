#include <auth/AuthModule.hpp>
#include <auth/controllers/AuthController.hpp>

#include <vix.hpp>

namespace cloud::auth
{
  const char *AuthModule::name()
  {
    return "auth";
  }

  void AuthModule::register_routes(vix::App &app)
  {
    controllers::AuthController::register_routes(app);
  }
} // namespace cloud::auth
