/**
 *
 *  @file MembersController.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Softadastra Cloud
 */
#include <members/controllers/MembersController.hpp>

#include <members/dto/MemberRequests.hpp>
#include <members/services/MemberService.hpp>
#include <members/support/MemberErrors.hpp>

#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::members::controllers
{
  namespace
  {
    services::MemberService &member_service()
    {
      static services::MemberService service;
      return service;
    }

    void json_error(
        vix::Response &res,
        int status,
        const std::string &code,
        const std::string &message)
    {
      res.status(status).json(
          vix::json::o(
              "ok", false,
              "error", code,
              "message", message));
    }

    void json_ok(
        vix::Response &res,
        const vix::json::Json &data)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", data));
    }

    void json_message(
        vix::Response &res,
        const std::string &message)
    {
      res.json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "message", message)));
    }

    bool require_json_object(
        const vix::json::Json &body,
        vix::Response &res)
    {
      if (body.is_object())
      {
        return true;
      }

      json_error(
          res,
          400,
          "invalid_request",
          "Expected JSON object body.");

      return false;
    }

    dto::InviteMemberRequest read_invite_member_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("user_id", ""),
          body.value("email", ""),
          body.value("role", ""),
          body.value("invited_by_user_id", ""),
      };
    }

    dto::UpdateMemberRoleRequest read_update_member_role_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("user_id", ""),
          body.value("role", ""),
      };
    }

    dto::RemoveMemberRequest read_remove_member_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
          body.value("user_id", ""),
      };
    }

    dto::ListMembersRequest read_list_members_request(
        const vix::json::Json &body)
    {
      return {
          body.value("workspace_id", ""),
      };
    }

    vix::json::Json member_list_to_json(
        const std::vector<dto::MemberResponse> &members)
    {
      auto items = vix::json::a();

      for (const auto &member : members)
      {
        items.push_back(member.to_json());
      }

      return items;
    }
  } // namespace

  void MembersController::register_routes(vix::App &app)
  {
    app.get("/api/members", [](vix::Request &req, vix::Response &res)
            {
      (void)req;

      json_message(
          res,
          "Members module is available"); });

    app.post("/api/members/invite", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_invite_member_request(body);
      auto invited = member_service().invite_member(request);

      if (invited.failed())
      {
        support::write_member_error(res, invited.error());
        return;
      }

      res.status(201).json(
          vix::json::o(
              "ok", true,
              "data", vix::json::o(
                  "member", invited.value().to_json()))); });

    app.post("/api/members/list", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_list_members_request(body);
      auto members = member_service().list_members(request);

      if (members.failed())
      {
        support::write_member_error(res, members.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "members", member_list_to_json(members.value()))); });

    app.post("/api/members/role", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_update_member_role_request(body);
      auto updated = member_service().update_member_role(request);

      if (updated.failed())
      {
        support::write_member_error(res, updated.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "member", updated.value().to_json())); });

    app.post("/api/members/remove", [](vix::Request &req, vix::Response &res)
             {
      const auto &body = req.json();

      if (!require_json_object(body, res))
      {
        return;
      }

      const auto request = read_remove_member_request(body);
      auto removed = member_service().remove_member(request);

      if (removed.failed())
      {
        support::write_member_error(res, removed.error());
        return;
      }

      json_ok(
          res,
          vix::json::o(
              "member", removed.value().to_json())); });
  }
} // namespace cloud::members::controllers
