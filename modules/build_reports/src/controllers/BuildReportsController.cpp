#include <build_reports/controllers/BuildReportsController.hpp>

#include <vix.hpp>

namespace cloud::build_reports::controllers
{
  void BuildReportsController::register_routes(vix::App &app)
  {
    app.get("/api/build_reports", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "build_reports",
        "message", "BuildReports module is available"
      });
    });
  }
} // namespace cloud::build_reports::controllers
