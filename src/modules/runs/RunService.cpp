/*
 * RunService.cpp
 */

#include "modules/runs/RunService.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "utils/IdGenerator.hpp"
#include "utils/Time.hpp"

namespace softadastra::cloud::modules::runs
{
  namespace
  {
    constexpr std::size_t MAX_TARGET_URL_LENGTH = 2048;
    constexpr std::size_t MIN_SCENARIO_LENGTH = 2;
    constexpr std::size_t MAX_SCENARIO_LENGTH = 120;

    [[nodiscard]] bool starts_with(
        const std::string &value,
        const std::string &prefix)
    {
      return value.rfind(prefix, 0) == 0;
    }
  }

  RunService::RunService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        repository_(state.database),
        project_repository_(state.database),
        agent_repository_(state.database)
  {
  }

  RunResult RunService::create_run(
      const std::string &project_public_id,
      const std::string &agent_public_id,
      const std::string &target_url,
      const std::string &method,
      const std::string &scenario)
  {
    if (project_public_id.empty())
    {
      return validation_error("Project id is required");
    }

    if (!is_valid_target_url(target_url))
    {
      return validation_error(
          "Target URL must start with http:// or https:// and contain at most 2048 characters");
    }

    const std::string normalized_method =
        normalize_method(method.empty() ? "GET" : method);

    if (!is_valid_method(normalized_method))
    {
      return validation_error(
          "HTTP method must be GET, POST, PUT, PATCH, DELETE, HEAD, or OPTIONS");
    }

    if (!is_valid_scenario(scenario))
    {
      return validation_error(
          "Scenario must contain between 2 and 120 characters");
    }

    const auto project =
        project_repository_.find_by_public_id(project_public_id);

    if (!project.has_value())
    {
      return not_found("Project not found");
    }

    std::int64_t agent_id = 0;

    if (!agent_public_id.empty())
    {
      const auto agent =
          agent_repository_.find_by_public_id(agent_public_id);

      if (!agent.has_value())
      {
        return not_found("Agent not found");
      }

      if (agent->project_id != project->id)
      {
        return validation_error("Agent does not belong to the selected project");
      }

      agent_id = agent->id;
    }

    const std::string public_id =
        softadastra::cloud::utils::IdGenerator::generate("run");

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool created =
        repository_.create(
            public_id,
            project->id,
            agent_id,
            "running",
            target_url,
            normalized_method,
            scenario,
            now,
            now);

    if (!created)
    {
      return internal_error("Failed to create run");
    }

    auto run =
        repository_.find_by_public_id(public_id);

    if (!run.has_value())
    {
      return internal_error("Run was created but could not be loaded");
    }

    RunResult result;
    result.success = true;
    result.status = 201;
    result.error.clear();
    result.message = "Run created";
    result.run = *run;

    return result;
  }

  RunListResult RunService::list_runs(
      int limit,
      int offset)
  {
    RunListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Runs loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.runs =
        repository_.all(
            result.limit,
            result.offset);
    result.total = repository_.count();

    return result;
  }

  RunListResult RunService::list_runs_for_project(
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

    RunListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Runs loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.runs =
        repository_.all_for_project(
            project_public_id,
            result.limit,
            result.offset);
    result.total =
        repository_.count_for_project(project_public_id);

    return result;
  }

  RunListResult RunService::list_runs_for_agent(
      const std::string &agent_public_id,
      int limit,
      int offset)
  {
    if (agent_public_id.empty())
    {
      return list_error(
          400,
          "validation_failed",
          "Agent id is required");
    }

    const auto agent =
        agent_repository_.find_by_public_id(agent_public_id);

    if (!agent.has_value())
    {
      return list_error(
          404,
          "agent_not_found",
          "Agent not found");
    }

    RunListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Runs loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.runs =
        repository_.all_for_agent(
            agent_public_id,
            result.limit,
            result.offset);
    result.total =
        repository_.count_for_agent(agent_public_id);

    return result;
  }

  RunResult RunService::get_run(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Run id is required");
    }

    auto run =
        repository_.find_by_public_id(public_id);

    if (!run.has_value())
    {
      return not_found("Run not found");
    }

    RunResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Run loaded";
    result.run = *run;

    return result;
  }

  RunResult RunService::update_status(
      const std::string &public_id,
      const std::string &status)
  {
    if (public_id.empty())
    {
      return validation_error("Run id is required");
    }

    if (!is_valid_status(status))
    {
      return validation_error("Invalid run status");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Run not found");
    }

    if (existing->is_finished())
    {
      return validation_error("Finished runs cannot be updated");
    }

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    bool updated = false;

    if (is_final_status(status))
    {
      updated =
          repository_.finish(
              public_id,
              status,
              now,
              now);
    }
    else
    {
      updated =
          repository_.update_status(
              public_id,
              status,
              now);
    }

    if (!updated)
    {
      return internal_error("Failed to update run status");
    }

    auto run =
        repository_.find_by_public_id(public_id);

    if (!run.has_value())
    {
      return internal_error("Run was updated but could not be loaded");
    }

    RunResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Run updated";
    result.run = *run;

    return result;
  }

  RunResult RunService::finish_run(
      const std::string &public_id,
      const std::string &status)
  {
    if (public_id.empty())
    {
      return validation_error("Run id is required");
    }

    const std::string final_status =
        status.empty() ? "finished" : status;

    if (!is_final_status(final_status))
    {
      return validation_error(
          "Final status must be finished, failed, or cancelled");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Run not found");
    }

    if (existing->is_finished())
    {
      RunResult result;
      result.success = true;
      result.status = 200;
      result.error.clear();
      result.message = "Run already finished";
      result.run = *existing;
      return result;
    }

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool finished =
        repository_.finish(
            public_id,
            final_status,
            now,
            now);

    if (!finished)
    {
      return internal_error("Failed to finish run");
    }

    auto run =
        repository_.find_by_public_id(public_id);

    if (!run.has_value())
    {
      return internal_error("Run was finished but could not be loaded");
    }

    RunResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Run finished";
    result.run = *run;

    return result;
  }

  RunResult RunService::delete_run(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Run id is required");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Run not found");
    }

    const bool deleted =
        repository_.remove(public_id);

    if (!deleted)
    {
      return internal_error("Failed to delete run");
    }

    RunResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Run deleted";
    result.run = *existing;

    return result;
  }

  bool RunService::is_valid_target_url(
      const std::string &target_url) noexcept
  {
    if (target_url.empty() ||
        target_url.size() > MAX_TARGET_URL_LENGTH)
    {
      return false;
    }

    return starts_with(target_url, "http://") ||
           starts_with(target_url, "https://");
  }

  bool RunService::is_valid_method(
      const std::string &method) noexcept
  {
    return method == "GET" ||
           method == "POST" ||
           method == "PUT" ||
           method == "PATCH" ||
           method == "DELETE" ||
           method == "HEAD" ||
           method == "OPTIONS";
  }

  bool RunService::is_valid_scenario(
      const std::string &scenario) noexcept
  {
    return scenario.size() >= MIN_SCENARIO_LENGTH &&
           scenario.size() <= MAX_SCENARIO_LENGTH;
  }

  bool RunService::is_valid_status(
      const std::string &status) noexcept
  {
    return status == "created" ||
           status == "running" ||
           status == "finished" ||
           status == "failed" ||
           status == "cancelled";
  }

  bool RunService::is_final_status(
      const std::string &status) noexcept
  {
    return status == "finished" ||
           status == "failed" ||
           status == "cancelled";
  }

  std::string RunService::normalize_method(
      const std::string &method)
  {
    std::string normalized = method;

    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c)
        {
          return static_cast<char>(std::toupper(c));
        });

    return normalized;
  }

  RunResult RunService::validation_error(
      std::string message)
  {
    RunResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  RunResult RunService::not_found(
      std::string message)
  {
    RunResult result;
    result.success = false;
    result.status = 404;
    result.error = "run_not_found";
    result.message = std::move(message);
    return result;
  }

  RunResult RunService::internal_error(
      std::string message)
  {
    RunResult result;
    result.success = false;
    result.status = 500;
    result.error = "internal_error";
    result.message = std::move(message);
    return result;
  }

  RunListResult RunService::list_error(
      int status,
      std::string error,
      std::string message)
  {
    RunListResult result;
    result.success = false;
    result.status = status;
    result.error = std::move(error);
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::runs
