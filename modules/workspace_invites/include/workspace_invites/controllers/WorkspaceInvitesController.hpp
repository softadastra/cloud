#ifndef cloud_workspace_invites_controller_hpp
#define cloud_workspace_invites_controller_hpp

namespace vix { class App; }

namespace cloud::workspace_invites::controllers
{
  class WorkspaceInvitesController
  {
  public:
    static void register_routes(vix::App &app);
  };
}

#endif
