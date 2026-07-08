#include <packages/PackagesModule.hpp>
#include <packages/controllers/PackagesController.hpp>

#include <vix.hpp>

namespace cloud::packages
{
  const char *PackagesModule::name()
  {
    return "packages";
  }

  void PackagesModule::register_routes(vix::App &app)
  {
    controllers::PackagesController::register_routes(app);
  }
} // namespace cloud::packages
