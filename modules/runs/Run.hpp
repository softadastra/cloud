/**
 *
 *  @file Run.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra/cloud
 *
 *  Licensed under the Apache License, Version 2.0.
 *
 *  Softadastra Cloud
 *
 */

#ifndef SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_HPP
#define SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::modules::runs
{
  /**
   * @brief Run model for Softadastra Reliability Cloud.
   *
   * A run represents one reliability test session executed by a Softadastra
   * agent against a target API endpoint.
   */
  struct Run
  {
    std::int64_t id{0};
    std::string public_id{};

    std::int64_t project_id{0};
    std::string project_public_id{};

    std::int64_t agent_id{0};
    std::string agent_public_id{};

    std::string status{"created"};
    std::string target_url{};
    std::string method{"GET"};
    std::string scenario{};

    std::int64_t started_at{0};
    std::int64_t finished_at{0};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return id > 0 &&
             !public_id.empty() &&
             project_id > 0 &&
             !target_url.empty() &&
             !method.empty() &&
             !scenario.empty();
    }

    [[nodiscard]] bool has_project() const noexcept
    {
      return project_id > 0 || !project_public_id.empty();
    }

    [[nodiscard]] bool has_agent() const noexcept
    {
      return agent_id > 0 || !agent_public_id.empty();
    }

    [[nodiscard]] bool is_finished() const noexcept
    {
      return status == "finished" ||
             status == "failed" ||
             status == "cancelled";
    }

    [[nodiscard]] bool is_running() const noexcept
    {
      return status == "running";
    }

    [[nodiscard]] bool has_finished_at() const noexcept
    {
      return finished_at > 0;
    }
  };

} // namespace softadastra::cloud::modules::runs

#endif // SOFTADASTRA_CLOUD_MODULES_RUNS_RUN_HPP
