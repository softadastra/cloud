/**
 *
 *  @file Project.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_HPP
#define SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::modules::projects
{
  /**
   * @brief Project model for Softadastra Reliability Cloud.
   *
   * A project represents one application, API, or backend that a user wants to
   * test under failure conditions.
   */
  struct Project
  {
    std::int64_t id{0};
    std::string public_id{};
    std::int64_t user_id{0};

    std::string name{};
    std::string slug{};
    std::string description{};
    std::string base_url{};

    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return id > 0 &&
             !public_id.empty() &&
             !name.empty() &&
             !slug.empty();
    }

    [[nodiscard]] bool has_owner() const noexcept
    {
      return user_id > 0;
    }

    [[nodiscard]] bool has_base_url() const noexcept
    {
      return !base_url.empty();
    }
  };

} // namespace softadastra::cloud::modules::projects

#endif // SOFTADASTRA_CLOUD_MODULES_PROJECTS_PROJECT_HPP
