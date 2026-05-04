/*
 * RequestIdMiddleware.cpp
 */

#include "middlewares/RequestIdMiddleware.hpp"

#include <chrono>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>

#include <vix/middleware/app/adapter.hpp>

#include "http/RequestContext.hpp"

namespace softadastra::cloud::middlewares
{
  namespace
  {
    [[nodiscard]] long long now_ms()
    {
      using namespace std::chrono;

      return static_cast<long long>(
          duration_cast<milliseconds>(
              system_clock::now().time_since_epoch())
              .count());
    }

    [[nodiscard]] std::string random_hex()
    {
      static thread_local std::mt19937_64 rng{
          std::random_device{}()};

      const std::uint64_t value = rng();

      std::ostringstream out;
      out << std::hex << value;

      return out.str();
    }

    [[nodiscard]] std::string make_request_id()
    {
      return "req_" +
             std::to_string(now_ms()) +
             "_" +
             random_hex();
    }
  }

  vix::App::Middleware RequestIdMiddleware::create()
  {
    return vix::middleware::app::adapt_ctx(
        [](vix::middleware::Context &ctx,
           vix::middleware::Next next)
        {
          softadastra::cloud::http::RequestContext request_context;
          request_context.request_id = make_request_id();

          ctx.req().emplace_state<softadastra::cloud::http::RequestContext>(
              request_context);

          ctx.res().header(
              "x-request-id",
              request_context.request_id);

          next();
        });
  }

} // namespace softadastra::cloud::middlewares
