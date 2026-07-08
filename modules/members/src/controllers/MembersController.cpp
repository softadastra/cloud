#include <members/controllers/MembersController.hpp>

#include <vix.hpp>

namespace cloud::members::controllers
{
  void MembersController::register_routes(vix::App &app)
  {
    app.get("/api/members", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "members",
        "message", "Members module is available"
      });
    });
  }
} // namespace cloud::members::controllers
