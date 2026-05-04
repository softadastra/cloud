/*
 * ProjectService.cpp
 */

#include "modules/projects/ProjectService.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include "utils/IdGenerator.hpp"
#include "utils/Time.hpp"

namespace softadastra::cloud::modules::projects
{
  namespace
  {
    constexpr std::size_t MIN_PROJECT_NAME_LENGTH = 2;
    constexpr std::size_t MAX_PROJECT_NAME_LENGTH = 120;
    constexpr std::size_t MAX_PROJECT_DESCRIPTION_LENGTH = 500;

    [[nodiscard]] bool starts_with(
        const std::string &value,
        const std::string &prefix)
    {
      return value.rfind(prefix, 0) == 0;
    }
  }

  ProjectService::ProjectService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        repository_(state.database)
  {
  }

  ProjectResult ProjectService::create_project(
      const std::string &name,
      const std::string &description,
      const std::string &base_url)
  {
    if (!is_valid_name(name))
    {
      return validation_error(
          "Project name must contain between 2 and 120 characters");
    }

    if (description.size() > MAX_PROJECT_DESCRIPTION_LENGTH)
    {
      return validation_error(
          "Project description must contain at most 500 characters");
    }

    if (!is_valid_base_url(base_url))
    {
      return validation_error(
          "Project base URL must start with http:// or https://");
    }

    const std::string public_id =
        softadastra::cloud::utils::IdGenerator::generate("prj");

    const std::string slug =
        make_unique_slug(name);

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool created =
        repository_.create(
            public_id,
            0,
            name,
            slug,
            description,
            base_url,
            now);

    if (!created)
    {
      return internal_error("Failed to create project");
    }

    auto project =
        repository_.find_by_public_id(public_id);

    if (!project.has_value())
    {
      return internal_error("Project was created but could not be loaded");
    }

    ProjectResult result;
    result.success = true;
    result.status = 201;
    result.error.clear();
    result.message = "Project created";
    result.project = *project;

    return result;
  }

  ProjectListResult ProjectService::list_projects(
      int limit,
      int offset)
  {
    ProjectListResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Projects loaded";
    result.limit = std::clamp(limit, 1, 100);
    result.offset = std::max(offset, 0);
    result.projects =
        repository_.all(
            result.limit,
            result.offset);
    result.total = repository_.count();

    return result;
  }

  ProjectResult ProjectService::get_project(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Project id is required");
    }

    auto project =
        repository_.find_by_public_id(public_id);

    if (!project.has_value())
    {
      return not_found("Project not found");
    }

    ProjectResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Project loaded";
    result.project = *project;

    return result;
  }

  ProjectResult ProjectService::update_project(
      const std::string &public_id,
      const std::string &name,
      const std::string &description,
      const std::string &base_url)
  {
    if (public_id.empty())
    {
      return validation_error("Project id is required");
    }

    if (!is_valid_name(name))
    {
      return validation_error(
          "Project name must contain between 2 and 120 characters");
    }

    if (description.size() > MAX_PROJECT_DESCRIPTION_LENGTH)
    {
      return validation_error(
          "Project description must contain at most 500 characters");
    }

    if (!is_valid_base_url(base_url))
    {
      return validation_error(
          "Project base URL must start with http:// or https://");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Project not found");
    }

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool updated =
        repository_.update(
            public_id,
            name,
            description,
            base_url,
            now);

    if (!updated)
    {
      return internal_error("Failed to update project");
    }

    auto project =
        repository_.find_by_public_id(public_id);

    if (!project.has_value())
    {
      return internal_error("Project was updated but could not be loaded");
    }

    ProjectResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Project updated";
    result.project = *project;

    return result;
  }

  ProjectResult ProjectService::delete_project(
      const std::string &public_id)
  {
    if (public_id.empty())
    {
      return validation_error("Project id is required");
    }

    auto existing =
        repository_.find_by_public_id(public_id);

    if (!existing.has_value())
    {
      return not_found("Project not found");
    }

    const bool deleted =
        repository_.remove(public_id);

    if (!deleted)
    {
      return internal_error("Failed to delete project");
    }

    ProjectResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Project deleted";
    result.project = *existing;

    return result;
  }

  std::string ProjectService::make_slug(
      const std::string &name)
  {
    std::string slug;
    slug.reserve(name.size());

    bool previous_dash = false;

    for (const unsigned char c : name)
    {
      if (std::isalnum(c) != 0)
      {
        slug.push_back(
            static_cast<char>(
                std::tolower(c)));

        previous_dash = false;
        continue;
      }

      if (!previous_dash && !slug.empty())
      {
        slug.push_back('-');
        previous_dash = true;
      }
    }

    while (!slug.empty() && slug.back() == '-')
    {
      slug.pop_back();
    }

    if (slug.empty())
    {
      return "project";
    }

    return slug;
  }

  std::string ProjectService::make_unique_slug(
      const std::string &name) const
  {
    const std::string base_slug = make_slug(name);

    std::string slug = base_slug;
    int suffix = 2;

    while (repository_.slug_exists(slug))
    {
      slug =
          base_slug +
          "-" +
          std::to_string(suffix);

      ++suffix;
    }

    return slug;
  }

  bool ProjectService::is_valid_name(
      const std::string &name) noexcept
  {
    return name.size() >= MIN_PROJECT_NAME_LENGTH &&
           name.size() <= MAX_PROJECT_NAME_LENGTH;
  }

  bool ProjectService::is_valid_base_url(
      const std::string &base_url) noexcept
  {
    if (base_url.empty())
    {
      return true;
    }

    return starts_with(base_url, "http://") ||
           starts_with(base_url, "https://");
  }

  ProjectResult ProjectService::validation_error(
      std::string message)
  {
    ProjectResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  ProjectResult ProjectService::not_found(
      std::string message)
  {
    ProjectResult result;
    result.success = false;
    result.status = 404;
    result.error = "project_not_found";
    result.message = std::move(message);
    return result;
  }

  ProjectResult ProjectService::conflict(
      std::string message)
  {
    ProjectResult result;
    result.success = false;
    result.status = 409;
    result.error = "conflict";
    result.message = std::move(message);
    return result;
  }

  ProjectResult ProjectService::internal_error(
      std::string message)
  {
    ProjectResult result;
    result.success = false;
    result.status = 500;
    result.error = "internal_error";
    result.message = std::move(message);
    return result;
  }

  ProjectListResult ProjectService::list_error(
      int status,
      std::string error,
      std::string message)
  {
    ProjectListResult result;
    result.success = false;
    result.status = status;
    result.error = std::move(error);
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::projects
