#ifndef cloud_workspace_invites_module_hpp
#define cloud_workspace_invites_module_hpp

namespace vix { class App; }

namespace cloud::workspace_invites
{
  class WorkspaceInvitesModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
}

#endif
