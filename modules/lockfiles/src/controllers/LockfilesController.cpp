#include <lockfiles/controllers/LockfilesController.hpp>

#include <vix.hpp>

namespace cloud::lockfiles::controllers
{
  void LockfilesController::register_routes(vix::App &app)
  {
    app.get("/api/lockfiles", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "lockfiles",
        "message", "Lockfiles module is available"
      });
    });
  }
} // namespace cloud::lockfiles::controllers
