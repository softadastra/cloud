#include <members/MembersModule.hpp>
#include <members/controllers/MembersController.hpp>

#include <vix.hpp>

namespace cloud::members
{
  const char *MembersModule::name()
  {
    return "members";
  }

  void MembersModule::register_routes(vix::App &app)
  {
    controllers::MembersController::register_routes(app);
  }
} // namespace cloud::members
