#ifndef cloud_tokens_module_hpp
#define cloud_tokens_module_hpp

namespace vix
{
  class App;
}

namespace cloud::tokens
{
  class TokensModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace cloud::tokens

#endif // cloud_tokens_module_hpp
