#include <projects/controllers/ProjectsController.hpp>

#include <vix.hpp>

namespace cloud::projects::controllers
{
  void ProjectsController::register_routes(vix::App &app)
  {
    app.get("/api/projects", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "projects",
        "message", "Projects module is available"
      });
    });
  }
} // namespace cloud::projects::controllers
