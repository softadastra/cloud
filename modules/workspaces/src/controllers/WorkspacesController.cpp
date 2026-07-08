#include <workspaces/controllers/WorkspacesController.hpp>

#include <vix.hpp>

namespace cloud::workspaces::controllers
{
  void WorkspacesController::register_routes(vix::App &app)
  {
    app.get("/api/workspaces", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "workspaces",
        "message", "Workspaces module is available"
      });
    });
  }
} // namespace cloud::workspaces::controllers
