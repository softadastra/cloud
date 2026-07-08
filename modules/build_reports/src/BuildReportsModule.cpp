#include <build_reports/BuildReportsModule.hpp>
#include <build_reports/controllers/BuildReportsController.hpp>

#include <vix.hpp>

namespace cloud::build_reports
{
  const char *BuildReportsModule::name()
  {
    return "build_reports";
  }

  void BuildReportsModule::register_routes(vix::App &app)
  {
    controllers::BuildReportsController::register_routes(app);
  }
} // namespace cloud::build_reports
