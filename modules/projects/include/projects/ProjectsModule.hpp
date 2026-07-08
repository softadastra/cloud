#ifndef cloud_projects_module_hpp
#define cloud_projects_module_hpp

namespace vix
{
  class App;
}

namespace cloud::projects
{
  class ProjectsModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::projects

#endif // cloud_projects_module_hpp
