#ifndef cloud_package_versions_controller_hpp
#define cloud_package_versions_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::package_versions::controllers
{
  class PackageVersionsController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::package_versions::controllers

#endif // cloud_package_versions_controller_hpp
