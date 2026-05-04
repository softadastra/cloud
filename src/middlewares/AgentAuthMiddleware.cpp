/*
 * AgentAuthMiddleware.cpp
 */

#include "middlewares/AgentAuthMiddleware.hpp"

#include <string>

#include <vix/log/Log.hpp>
#include <vix/middleware/app/adapter.hpp>

#include "http/JsonResponse.hpp"
#include "http/RequestContext.hpp"

namespace softadastra::cloud::middlewares
{
  namespace
  {
    constexpr const char *AGENT_API_KEY_HEADER = "x-agent-api-key";

    [[nodiscard]] bool constant_time_equals(
        const std::string &a,
        const std::string &b) noexcept
    {
      if (a.size() != b.size())
      {
        return false;
      }

      unsigned char diff = 0;

      for (std::size_t i = 0; i < a.size(); ++i)
      {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
      }

      return diff == 0;
    }

    void attach_agent_context(
        vix::Request &req)
    {
      auto *context =
          req.try_state<softadastra::cloud::http::RequestContext>();

      if (context == nullptr)
      {
        softadastra::cloud::http::RequestContext fresh_context;
        fresh_context.set_agent_auth(
            "local-agent",
            "local-project");

        req.emplace_state<softadastra::cloud::http::RequestContext>(
            fresh_context);

        return;
      }

      context->set_agent_auth(
          "local-agent",
          "local-project");
    }
  }

  vix::App::Middleware AgentAuthMiddleware::create(
      const softadastra::cloud::app::AppState &state)
  {
    return vix::middleware::app::adapt_ctx(
        [&state](vix::middleware::Context &ctx,
                 vix::middleware::Next next)
        {
          const std::string expected_key =
              state.config.agent_api_key_secret;

          if (expected_key.empty())
          {
            vix::log::error(
                "agent authentication failed: missing configured API key secret");

            softadastra::cloud::http::JsonResponse::internal_error(
                ctx.res(),
                "Agent authentication is not configured");

            return;
          }

          const std::string provided_key =
              ctx.req().header(AGENT_API_KEY_HEADER);

          if (provided_key.empty())
          {
            softadastra::cloud::http::JsonResponse::unauthorized(
                ctx.res(),
                "Missing agent API key");

            return;
          }

          if (!constant_time_equals(provided_key, expected_key))
          {
            vix::log::warn("agent authentication failed: invalid API key");

            softadastra::cloud::http::JsonResponse::unauthorized(
                ctx.res(),
                "Invalid agent API key");

            return;
          }

          attach_agent_context(ctx.req());

          next();
        });
  }

} // namespace softadastra::cloud::middlewares
