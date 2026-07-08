#ifndef cloud_packages_module_hpp
#define cloud_packages_module_hpp

namespace vix
{
  class App;
}

namespace cloud::packages
{
  class PackagesModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::packages

#endif // cloud_packages_module_hpp
