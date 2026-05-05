/*
 * AgentService.cpp
 */

#include "modules/agents/AgentService.hpp"

#include <algorithm>
#include <string>

#include <vix/log/Log.hpp>

#include "utils/IdGenerator.hpp"
#include "utils/Time.hpp"
#include "security/ApiKeyHasher.hpp"

namespace softadastra::cloud::modules::agents
{
  namespace
  {
    constexpr std::size_t MIN_AGENT_NAME_LENGTH = 2;
    constexpr std::size_t MAX_AGENT_NAME_LENGTH = 120;
  }

  AgentService::AgentService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        repository_(state.database),
        project_repository_(state.database)
  {
  }

  AgentResult AgentService::create_agent(
      const std::string &project_public_id,
      const std::string &name)
  {
    if (project_public_id.empty())
    {
      return validation_error("Project id is required");
    }

    if (!is_valid_name(name))
    {
      return validation_error(
          "Agent name must contain between 2 and 120 characters");
    }

    const auto project =
        project_repository_.find_by_public_id(project_public_id);

    if (!project.has_value())
    {
      return not_found("Project not found");
    }

    const std::string public_id =
        softadastra::cloud::utils::IdGenerator::generate("agt");

    const std::string api_key =
        generate_api_key();

    const std::string api_key_hash =
        hash_api_key(
            state_,
            api_key);

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool created =
        repository_.create(
            public_id,
            project->id,
            name,
            api_key_hash,
            now);

    if (!created)
    {
      return internal_error("Failed to create agent");
    }

    auto agent =
        repository_.find_by_public_id(public_id);

    if (!agent.has_value())
    {
      return internal_error("Agent was created but could not be loaded");
    }

    AgentResult result;
    result.success = true;
    result.status = 201;
    result.error.clear();
    result.message = "Agent created";
    result.agent = *agent;
    result.api_key = api_key;

    return result;
  }

  AgentListResult AgentService::list_agents(
      int limit,
      int offset)
  {
    AgentListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Agents loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.agents =
        repository_.all(
            result.limit,
            result.offset);
    result.total = repository_.count();

    return result;
  }

  AgentListResult AgentService::list_agents_for_project(
      const std::string &project_public_id,
      int limit,
      int offset)
  {
    if (project_public_id.empty())
    {
      return list_error(
          400,
          "validation_failed",
          "Project id is required");
    }

    const auto project =
        project_repository_.find_by_public_id(project_public_id);

    if (!project.has_value())
    {
      return list_error(
          404,
          "project_not_found",
          "Project not found");
    }

    AgentListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Agents loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.agents =
        repository_.all_for_project(
            project_public_id,
            result.limit,
            result.offset);
    result.total =
        repository_.count_for_project(project_public_id);

    return result;
  }

  AgentResult AgentService::get_agent(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Agent id is required");
    }

    auto agent =
        repository_.find_by_public_id(public_id);

    if (!agent.has_value())
    {
      return not_found("Agent not found");
    }

    AgentResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Agent loaded";
    result.agent = *agent;

    return result;
  }

  AgentResult AgentService::rotate_api_key(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Agent id is required");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Agent not found");
    }

    const std::string api_key =
        generate_api_key();

    const std::string api_key_hash =
        hash_api_key(
            state_,
            api_key);

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool rotated =
        repository_.rotate_api_key(
            public_id,
            api_key_hash,
            now);

    if (!rotated)
    {
      return internal_error("Failed to rotate agent API key");
    }

    auto agent =
        repository_.find_by_public_id(public_id);

    if (!agent.has_value())
    {
      return internal_error("Agent key was rotated but agent could not be loaded");
    }

    AgentResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Agent API key rotated";
    result.agent = *agent;
    result.api_key = api_key;

    return result;
  }

  AgentResult AgentService::delete_agent(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Agent id is required");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Agent not found");
    }

    const bool deleted =
        repository_.remove(public_id);

    if (!deleted)
    {
      return internal_error("Failed to delete agent");
    }

    AgentResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Agent deleted";
    result.agent = *existing;

    return result;
  }

  AgentResult AgentService::authenticate_api_key(
      const std::string &api_key)
  {
    if (api_key.empty())
    {
      return unauthorized("Missing agent API key");
    }

    const std::string api_key_hash =
        hash_api_key(
            state_,
            api_key);

    auto agent =
        repository_.find_by_api_key_hash(api_key_hash);

    if (!agent.has_value())
    {
      return unauthorized("Invalid agent API key");
    }

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    repository_.touch_last_seen(
        agent->public_id,
        now);

    auto fresh_agent =
        repository_.find_by_public_id(agent->public_id);

    AgentResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Agent authenticated";
    result.agent = fresh_agent.value_or(*agent);

    return result;
  }

  bool AgentService::is_valid_name(
      const std::string &name) noexcept
  {
    return name.size() >= MIN_AGENT_NAME_LENGTH &&
           name.size() <= MAX_AGENT_NAME_LENGTH;
  }

  std::string AgentService::generate_api_key()
  {
    return softadastra::cloud::security::ApiKeyHasher::generate_agent_key();
  }

  std::string AgentService::hash_api_key(
      const softadastra::cloud::app::AppState &state,
      const std::string &api_key)
  {
    return softadastra::cloud::security::ApiKeyHasher::hash(
        state.config,
        api_key);
  }

  AgentResult AgentService::validation_error(
      std::string message)
  {
    AgentResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  AgentResult AgentService::unauthorized(
      std::string message)
  {
    AgentResult result;
    result.success = false;
    result.status = 401;
    result.error = "unauthorized";
    result.message = std::move(message);
    return result;
  }

  AgentResult AgentService::not_found(
      std::string message)
  {
    AgentResult result;
    result.success = false;
    result.status = 404;
    result.error = "agent_not_found";
    result.message = std::move(message);
    return result;
  }

  AgentResult AgentService::internal_error(
      std::string message)
  {
    AgentResult result;
    result.success = false;
    result.status = 500;
    result.error = "internal_error";
    result.message = std::move(message);
    return result;
  }

  AgentListResult AgentService::list_error(
      int status,
      std::string error,
      std::string message)
  {
    AgentListResult result;
    result.success = false;
    result.status = status;
    result.error = std::move(error);
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::agents
