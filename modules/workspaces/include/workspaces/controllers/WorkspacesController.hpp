#ifndef cloud_workspaces_controller_hpp
#define cloud_workspaces_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::workspaces::controllers
{
  class WorkspacesController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::workspaces::controllers

#endif // cloud_workspaces_controller_hpp
