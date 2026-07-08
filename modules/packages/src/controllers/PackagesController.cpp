#include <packages/controllers/PackagesController.hpp>

#include <vix.hpp>

namespace cloud::packages::controllers
{
  void PackagesController::register_routes(vix::App &app)
  {
    app.get("/api/packages", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "packages",
        "message", "Packages module is available"
      });
    });
  }
} // namespace cloud::packages::controllers
