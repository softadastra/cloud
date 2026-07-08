#include <lockfiles/LockfilesModule.hpp>
#include <lockfiles/controllers/LockfilesController.hpp>

#include <vix.hpp>

namespace cloud::lockfiles
{
  const char *LockfilesModule::name()
  {
    return "lockfiles";
  }

  void LockfilesModule::register_routes(vix::App &app)
  {
    controllers::LockfilesController::register_routes(app);
  }
} // namespace cloud::lockfiles
