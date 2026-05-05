/*
 * AgentAuthMiddleware.cpp
 */

#include "middlewares/AgentAuthMiddleware.hpp"

#include <string>

#include <vix/log/Log.hpp>
#include <vix/middleware/app/adapter.hpp>

#include "http/JsonResponse.hpp"
#include "http/RequestContext.hpp"
#include "modules/agents/AgentService.hpp"

namespace softadastra::cloud::middlewares
{
  namespace
  {
    constexpr const char *AGENT_API_KEY_HEADER = "x-agent-api-key";

    [[nodiscard]] std::string read_agent_api_key(
        const vix::Request &req)
    {
      std::string api_key =
          req.header(AGENT_API_KEY_HEADER);

      if (!api_key.empty())
      {
        return api_key;
      }

      /*
       * Compatibility with the API documentation.
       */
      api_key = req.header("X-Agent-Key");

      return api_key;
    }

    void attach_agent_context(
        vix::Request &req,
        const softadastra::cloud::modules::agents::Agent &agent)
    {
      auto *context =
          req.try_state<softadastra::cloud::http::RequestContext>();

      if (context == nullptr)
      {
        softadastra::cloud::http::RequestContext fresh_context;

        fresh_context.set_agent_auth(
            agent.public_id,
            agent.project_public_id);

        req.emplace_state<softadastra::cloud::http::RequestContext>(
            fresh_context);

        return;
      }

      context->set_agent_auth(
          agent.public_id,
          agent.project_public_id);
    }
  }

  vix::App::Middleware AgentAuthMiddleware::create(
      const softadastra::cloud::app::AppState &state)
  {
    return vix::middleware::app::adapt_ctx(
        [&state](vix::middleware::Context &ctx,
                 vix::middleware::Next next)
        {
          const std::string provided_key =
              read_agent_api_key(ctx.req());

          if (provided_key.empty())
          {
            softadastra::cloud::http::JsonResponse::unauthorized(
                ctx.res(),
                "Missing agent API key");

            return;
          }

          softadastra::cloud::modules::agents::AgentService service{state};

          const auto result =
              service.authenticate_api_key(provided_key);

          if (!result.success)
          {
            vix::log::warn("agent authentication failed: invalid API key");

            softadastra::cloud::http::JsonResponse::error(
                ctx.res(),
                result.status,
                result.error,
                result.message);

            return;
          }

          attach_agent_context(
              ctx.req(),
              result.agent);

          next();
        });
  }

} // namespace softadastra::cloud::middlewares
