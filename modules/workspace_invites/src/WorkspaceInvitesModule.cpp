#include <workspace_invites/WorkspaceInvitesModule.hpp>
#include <workspace_invites/controllers/WorkspaceInvitesController.hpp>

#include <vix.hpp>

namespace cloud::workspace_invites
{
  const char *WorkspaceInvitesModule::name()
  {
    return "workspace_invites";
  }

  void WorkspaceInvitesModule::register_routes(vix::App &app)
  {
    controllers::WorkspaceInvitesController::register_routes(app);
  }
}
