#include <projects/ProjectsModule.hpp>
#include <projects/controllers/ProjectsController.hpp>

#include <vix.hpp>

namespace cloud::projects
{
  const char *ProjectsModule::name()
  {
    return "projects";
  }

  void ProjectsModule::register_routes(vix::App &app)
  {
    controllers::ProjectsController::register_routes(app);
  }
} // namespace cloud::projects
