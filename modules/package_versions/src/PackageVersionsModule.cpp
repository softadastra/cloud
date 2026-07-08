#include <package_versions/PackageVersionsModule.hpp>
#include <package_versions/controllers/PackageVersionsController.hpp>

#include <vix.hpp>

namespace cloud::package_versions
{
  const char *PackageVersionsModule::name()
  {
    return "package_versions";
  }

  void PackageVersionsModule::register_routes(vix::App &app)
  {
    controllers::PackageVersionsController::register_routes(app);
  }
} // namespace cloud::package_versions
