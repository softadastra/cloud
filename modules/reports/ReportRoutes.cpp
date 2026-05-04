/*
 * ReportRoutes.cpp
 */

#include "modules/reports/ReportRoutes.hpp"
#include "modules/reports/ReportController.hpp"

namespace softadastra::cloud::modules::reports
{
  void ReportRoutes::register_routes(
      vix::App &app,
      const softadastra::cloud::app::AppState &state)
  {
    app.get(
        "/api/reports",
        [&state](vix::Request &req, vix::Response &res)
        {
          ReportController::list(req, res, state);
        });

    app.get(
        "/api/reports/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          ReportController::get(req, res, state);
        });

    app.del(
        "/api/reports/{id}",
        [&state](vix::Request &req, vix::Response &res)
        {
          ReportController::remove(req, res, state);
        });

    app.get(
        "/api/runs/{id}/report",
        [&state](vix::Request &req, vix::Response &res)
        {
          ReportController::get_for_run(req, res, state);
        });

    app.post(
        "/api/runs/{id}/report",
        [&state](vix::Request &req, vix::Response &res)
        {
          ReportController::submit_for_run(req, res, state);
        });

    app.del(
        "/api/runs/{id}/report",
        [&state](vix::Request &req, vix::Response &res)
        {
          ReportController::remove_for_run(req, res, state);
        });
  }

} // namespace softadastra::cloud::modules::reports
