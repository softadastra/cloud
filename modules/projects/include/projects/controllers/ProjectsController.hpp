#ifndef cloud_projects_controller_hpp
#define cloud_projects_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::projects::controllers
{
  class ProjectsController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::projects::controllers

#endif // cloud_projects_controller_hpp
