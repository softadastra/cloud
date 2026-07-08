#include <tokens/controllers/TokensController.hpp>

#include <vix.hpp>

namespace cloud::tokens::controllers
{
  void TokensController::register_routes(vix::App &app)
  {
    app.get("/api/tokens", [](vix::Request &req, vix::Response &res)
    {
      (void)req;

      res.json({
        "ok", true,
        "module", "tokens",
        "message", "Tokens module is available"
      });
    });
  }
} // namespace cloud::tokens::controllers
