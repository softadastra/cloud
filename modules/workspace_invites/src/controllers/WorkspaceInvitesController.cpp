#include <workspace_invites/controllers/WorkspaceInvitesController.hpp>

#include <workspace_invites/dto/WorkspaceInviteRequests.hpp>
#include <workspace_invites/services/WorkspaceInviteService.hpp>
#include <workspace_invites/support/WorkspaceInviteErrors.hpp>

#include <string>
#include <vector>

#include <vix.hpp>
#include <vix/json.hpp>

namespace cloud::workspace_invites::controllers
{
  namespace
  {
    services::WorkspaceInviteService &workspace_invite_service()
    {
      static services::WorkspaceInviteService service;
      return service;
    }

    void json_error(vix::Response &res, int status, const std::string &code, const std::string &message)
    {
      res.status(status).json(vix::json::o("ok", false, "error", code, "message", message));
    }

    void json_ok(vix::Response &res, const vix::json::Json &data)
    {
      res.json(vix::json::o("ok", true, "data", data));
    }

    void json_message(vix::Response &res, const std::string &message)
    {
      res.json(vix::json::o("ok", true, "data", vix::json::o("message", message)));
    }

    bool require_json_object(const vix::json::Json &body, vix::Response &res)
    {
      if (body.is_object())
      {
        return true;
      }
      json_error(res, 400, "invalid_request", "Expected JSON object body.");
      return false;
    }

    vix::json::Json invite_list_to_json(const std::vector<dto::WorkspaceInviteResponse> &invites)
    {
      auto items = vix::json::a();
      for (const auto &invite : invites)
      {
        items.push_back(invite.to_json());
      }
      return items;
    }
  }

  void WorkspaceInvitesController::register_routes(vix::App &app)
  {
    app.get("/api/workspace_invites", [](vix::Request &req, vix::Response &res)
    {
      (void)req;
      json_message(res, "Workspace invites module is available");
    });

    app.post("/api/workspace_invites/create", [](vix::Request &req, vix::Response &res)
    {
      const auto &body = req.json();
      if (!require_json_object(body, res)) return;

      dto::CreateWorkspaceInviteRequest request;
      request.workspace_id = body.value("workspace_id", "");
      request.invited_email = body.value("invited_email", body.value("email", ""));
      request.role = body.value("role", "");
      request.invited_by_user_id = body.value("invited_by_user_id", "");
      request.access_scope = body.value("access_scope", "");
      request.project_ids_json = body.value("project_ids_json", body.value("project_ids", ""));
      request.expires_at = body.value("expires_at", static_cast<std::int64_t>(0));

      auto created = workspace_invite_service().create_invite(request);
      if (created.failed())
      {
        support::write_workspace_invite_error(res, created.error());
        return;
      }
      res.status(201).json(vix::json::o("ok", true, "data", vix::json::o("invite", created.value().to_json())));
    });

    app.post("/api/workspace_invites/list", [](vix::Request &req, vix::Response &res)
    {
      const auto &body = req.json();
      if (!require_json_object(body, res)) return;

      dto::ListWorkspaceInvitesRequest request{body.value("workspace_id", "")};
      auto invites = workspace_invite_service().list_invites(request);
      if (invites.failed())
      {
        support::write_workspace_invite_error(res, invites.error());
        return;
      }
      json_ok(res, vix::json::o("invites", invite_list_to_json(invites.value())));
    });

    app.post("/api/workspace_invites/list_mine", [](vix::Request &req, vix::Response &res)
    {
      const auto &body = req.json();
      if (!require_json_object(body, res)) return;

      dto::ListMyWorkspaceInvitesRequest request;
      request.user_id = body.value("user_id", "");
      request.email = body.value("email", "");
      auto invites = workspace_invite_service().list_my_invites(request);
      if (invites.failed())
      {
        support::write_workspace_invite_error(res, invites.error());
        return;
      }
      json_ok(res, vix::json::o("invites", invite_list_to_json(invites.value())));
    });

    app.post("/api/workspace_invites/accept", [](vix::Request &req, vix::Response &res)
    {
      const auto &body = req.json();
      if (!require_json_object(body, res)) return;

      dto::WorkspaceInviteActionRequest request;
      request.invite_id = body.value("invite_id", "");
      request.user_id = body.value("user_id", "");
      request.email = body.value("email", "");
      auto accepted = workspace_invite_service().accept_invite(request);
      if (accepted.failed())
      {
        support::write_workspace_invite_error(res, accepted.error());
        return;
      }
      json_ok(res, vix::json::o("invite", accepted.value().to_json()));
    });

    app.post("/api/workspace_invites/decline", [](vix::Request &req, vix::Response &res)
    {
      const auto &body = req.json();
      if (!require_json_object(body, res)) return;

      dto::WorkspaceInviteActionRequest request;
      request.invite_id = body.value("invite_id", "");
      request.user_id = body.value("user_id", "");
      request.email = body.value("email", "");
      auto declined = workspace_invite_service().decline_invite(request);
      if (declined.failed())
      {
        support::write_workspace_invite_error(res, declined.error());
        return;
      }
      json_ok(res, vix::json::o("invite", declined.value().to_json()));
    });

    app.post("/api/workspace_invites/revoke", [](vix::Request &req, vix::Response &res)
    {
      const auto &body = req.json();
      if (!require_json_object(body, res)) return;

      dto::RevokeWorkspaceInviteRequest request;
      request.workspace_id = body.value("workspace_id", "");
      request.invite_id = body.value("invite_id", "");
      request.revoked_by_user_id = body.value("revoked_by_user_id", "");
      auto revoked = workspace_invite_service().revoke_invite(request);
      if (revoked.failed())
      {
        support::write_workspace_invite_error(res, revoked.error());
        return;
      }
      json_ok(res, vix::json::o("invite", revoked.value().to_json()));
    });
  }
}
