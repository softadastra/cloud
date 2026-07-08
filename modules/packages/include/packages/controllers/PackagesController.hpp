#ifndef cloud_packages_controller_hpp
#define cloud_packages_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::packages::controllers
{
  class PackagesController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::packages::controllers

#endif // cloud_packages_controller_hpp
