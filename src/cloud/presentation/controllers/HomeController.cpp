/**
 * @file HomeController.cpp
 * @brief Home route implementation for the cloud backend.
 */

#include <cloud/presentation/controllers/HomeController.hpp>

#include <vix.hpp>

namespace cloud::presentation::controllers
{
  void HomeController::register_routes(vix::App &app)
  {
    app.get("/api", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "service", "cloud",
        "message", "Vix backend is running"
      });
    });
  }
} // namespace cloud::presentation::controllers
