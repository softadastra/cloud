/*
 * ProjectController.cpp
 */

#include "modules/projects/ProjectController.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

#include <vix/log/Log.hpp>
#include <vix/json/json.hpp>
#include <vix/json/convert.hpp>

#include "http/JsonResponse.hpp"
#include "modules/projects/ProjectService.hpp"

namespace softadastra::cloud::modules::projects
{
  namespace J = vix::json;

  namespace
  {
    [[nodiscard]] bool has_string_field(
        const J::Json &body,
        std::string_view field)
    {
      return body.is_object() &&
             body.contains(std::string(field)) &&
             body.at(std::string(field)).is_string();
    }

    [[nodiscard]] std::string string_field_or_empty(
        const J::Json &body,
        std::string_view field)
    {
      return J::get_or<std::string>(
          body,
          field,
          "");
    }

    [[nodiscard]] int int_query_or(
        const vix::Request &req,
        const std::string &key,
        int fallback)
    {
      const std::string value = req.query_value(key, "");

      if (value.empty())
      {
        return fallback;
      }

      try
      {
        return std::stoi(value);
      }
      catch (...)
      {
        return fallback;
      }
    }

    [[nodiscard]] J::token project_to_json(
        const Project &project)
    {
      return J::obj({
          "id",
          project.public_id,
          "name",
          project.name,
          "slug",
          project.slug,
          "description",
          project.description,
          "base_url",
          project.base_url,
          "created_at",
          project.created_at,
          "updated_at",
          project.updated_at,
      });
    }

    [[nodiscard]] J::token projects_to_json(
        const std::vector<Project> &projects)
    {
      std::vector<J::token> items;
      items.reserve(projects.size());

      for (const auto &project : projects)
      {
        items.push_back(project_to_json(project));
      }

      return J::array(std::move(items));
    }

    void respond_project_result(
        vix::Response &res,
        const ProjectResult &result)
    {
      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          project_to_json(result.project));
    }
  }

  void ProjectController::create(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const auto &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      const std::string name =
          string_field_or_empty(body, "name");

      const std::string description =
          string_field_or_empty(body, "description");

      const std::string base_url =
          string_field_or_empty(body, "base_url");

      ProjectService service{state};

      const auto result =
          service.create_project(
              name,
              description,
              base_url);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::created(
          res,
          project_to_json(result.project));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "create project failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ProjectController::list(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const int limit =
          std::clamp(
              int_query_or(req, "limit", 20),
              1,
              100);

      const int offset =
          std::max(
              int_query_or(req, "offset", 0),
              0);

      ProjectService service{state};

      const auto result =
          service.list_projects(
              limit,
              offset);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          J::obj({
              "count",
              static_cast<long long>(result.projects.size()),
              "total",
              result.total,
              "limit",
              result.limit,
              "offset",
              result.offset,
              "items",
              projects_to_json(result.projects),
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "list projects failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ProjectController::get(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string project_id =
          req.param("id", "");

      ProjectService service{state};

      const auto result =
          service.get_project(project_id);

      respond_project_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "get project failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ProjectController::update(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string project_id =
          req.param("id", "");

      const auto &body = req.json();

      if (!body.is_object())
      {
        softadastra::cloud::http::JsonResponse::validation_error(
            res,
            "Expected JSON object body");

        return;
      }

      const std::string name =
          string_field_or_empty(body, "name");

      const std::string description =
          string_field_or_empty(body, "description");

      const std::string base_url =
          string_field_or_empty(body, "base_url");

      ProjectService service{state};

      const auto result =
          service.update_project(
              project_id,
              name,
              description,
              base_url);

      respond_project_result(res, result);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "update project failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

  void ProjectController::remove(
      vix::Request &req,
      vix::Response &res,
      const softadastra::cloud::app::AppState &state)
  {
    try
    {
      const std::string project_id =
          req.param("id", "");

      ProjectService service{state};

      const auto result =
          service.delete_project(project_id);

      if (!result.success)
      {
        softadastra::cloud::http::JsonResponse::error(
            res,
            result.status,
            result.error,
            result.message);

        return;
      }

      softadastra::cloud::http::JsonResponse::data(
          res,
          result.message,
          J::obj({
              "id",
              result.project.public_id,
              "deleted",
              true,
          }));
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "delete project failed",
          "error", e.what());

      softadastra::cloud::http::JsonResponse::internal_error(res);
    }
  }

} // namespace softadastra::cloud::modules::projects
