#ifndef cloud_tokens_controller_hpp
#define cloud_tokens_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::tokens::controllers
{
  class TokensController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::tokens::controllers

#endif // cloud_tokens_controller_hpp
