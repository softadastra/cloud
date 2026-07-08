#ifndef cloud_build_reports_module_hpp
#define cloud_build_reports_module_hpp

namespace vix
{
  class App;
}

namespace cloud::build_reports
{
  class BuildReportsModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::build_reports

#endif // cloud_build_reports_module_hpp
