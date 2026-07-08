#include <workspaces/WorkspacesModule.hpp>
#include <workspaces/controllers/WorkspacesController.hpp>

#include <vix.hpp>

namespace cloud::workspaces
{
  const char *WorkspacesModule::name()
  {
    return "workspaces";
  }

  void WorkspacesModule::register_routes(vix::App &app)
  {
    controllers::WorkspacesController::register_routes(app);
  }
} // namespace cloud::workspaces
