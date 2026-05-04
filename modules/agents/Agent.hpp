/**
 *
 *  @file Agent.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_HPP
#define SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::modules::agents
{
  /**
   * @brief Agent model for Softadastra Reliability Cloud.
   *
   * An agent represents a CLI/runtime instance allowed to send runs, events,
   * and reports for one project.
   */
  struct Agent
  {
    std::int64_t id{0};
    std::string public_id{};

    std::int64_t project_id{0};
    std::string project_public_id{};

    std::string name{};
    std::string api_key_hash{};

    std::int64_t last_seen_at{0};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return id > 0 &&
             !public_id.empty() &&
             project_id > 0 &&
             !name.empty();
    }

    [[nodiscard]] bool has_project() const noexcept
    {
      return project_id > 0 || !project_public_id.empty();
    }

    [[nodiscard]] bool has_seen_activity() const noexcept
    {
      return last_seen_at > 0;
    }
  };

} // namespace softadastra::cloud::modules::agents

#endif // SOFTADASTRA_CLOUD_MODULES_AGENTS_AGENT_HPP
