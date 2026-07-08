#include <tokens/TokensModule.hpp>
#include <tokens/controllers/TokensController.hpp>

#include <vix.hpp>

namespace cloud::tokens
{
  const char *TokensModule::name()
  {
    return "tokens";
  }

  void TokensModule::register_routes(vix::App &app)
  {
    controllers::TokensController::register_routes(app);
  }
} // namespace cloud::tokens
