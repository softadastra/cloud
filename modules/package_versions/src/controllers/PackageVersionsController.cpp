#include <package_versions/controllers/PackageVersionsController.hpp>

#include <vix.hpp>

namespace cloud::package_versions::controllers
{
  void PackageVersionsController::register_routes(vix::App &app)
  {
    app.get("/api/package_versions", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "package_versions",
        "message", "PackageVersions module is available"
      });
    });
  }
} // namespace cloud::package_versions::controllers
