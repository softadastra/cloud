#ifndef cloud_workspaces_module_hpp
#define cloud_workspaces_module_hpp

namespace vix
{
  class App;
}

namespace cloud::workspaces
{
  class WorkspacesModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::workspaces

#endif // cloud_workspaces_module_hpp
