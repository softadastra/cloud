/*
 * RequestContext.cpp
 */

#include "http/RequestContext.hpp"

#include <utility>

namespace softadastra::cloud::http
{
  bool RequestContext::has_request_id() const noexcept
  {
    return !request_id.empty();
  }

  bool RequestContext::has_agent() const noexcept
  {
    return !agent_public_id.empty();
  }

  bool RequestContext::has_project() const noexcept
  {
    return !project_public_id.empty();
  }

  void RequestContext::set_agent_auth(
      std::string agent_id,
      std::string project_id)
  {
    agent_public_id = std::move(agent_id);
    project_public_id = std::move(project_id);
    auth_source = "agent_api_key";
  }

  void RequestContext::clear_auth() noexcept
  {
    agent_public_id.clear();
    project_public_id.clear();
    auth_source = "none";
  }

} // namespace softadastra::cloud::http
