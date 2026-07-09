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

      res.json(vix::json::o(
          "ok", true,
          "data", vix::json::o(
              "service", "cloud",
              "message", "Vix backend is running")));
    });
  }
} // namespace cloud::presentation::controllers
