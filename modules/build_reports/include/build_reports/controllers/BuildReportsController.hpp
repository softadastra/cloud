#ifndef cloud_build_reports_controller_hpp
#define cloud_build_reports_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::build_reports::controllers
{
  class BuildReportsController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::build_reports::controllers

#endif // cloud_build_reports_controller_hpp
