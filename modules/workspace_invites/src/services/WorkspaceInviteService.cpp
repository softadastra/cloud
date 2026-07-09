#include <workspace_invites/services/WorkspaceInviteService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::workspace_invites::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_invite_id()
    {
      static std::atomic<std::uint64_t> counter{0};
      std::ostringstream stream;
      stream << "workspace_invite_" << now_timestamp() << "_" << counter.fetch_add(1);
      return stream.str();
    }

    bool valid_role(const std::string &role)
    {
      return role == "admin" || role == "member" || role == "viewer";
    }

    std::string normalize_role(const std::string &role)
    {
      return role.empty() ? "member" : role;
    }

    bool valid_access_scope(const std::string &scope)
    {
      return scope == "entire_workspace" || scope == "selected_projects";
    }

    std::string normalize_access_scope(const std::string &scope)
    {
      return scope.empty() ? "entire_workspace" : scope;
    }

    bool selected_projects_empty(const std::string &project_ids_json)
    {
      return project_ids_json.empty() || project_ids_json == "[]";
    }
  }

  class WorkspaceInviteService::Impl
  {
  public:
    Impl()
    {
      const char *use_db = std::getenv("SOFTADASTRA_CLOUD_USE_DB");
      if (use_db != nullptr && std::string(use_db) == "1")
      {
        vix::config::Config cfg{".env"};
        db = std::make_unique<vix::db::Database>(cfg);
      }
    }

    bool persistent() const { return db != nullptr; }

    dto::WorkspaceInviteResponse row_to_invite(const vix::db::ResultRow &row) const
    {
      dto::WorkspaceInviteResponse invite;
      invite.id = row.getString(0);
      invite.workspace_id = row.getString(1);
      invite.workspace_name = row.getString(2);
      invite.invited_email = row.getString(3);
      invite.invited_user_id = row.getString(4);
      invite.role = row.getString(5);
      invite.invited_by_user_id = row.getString(6);
      invite.status = row.getString(7);
      invite.access_scope = row.getString(8);
      invite.project_ids_json = row.getString(9);
      invite.created_at = row.getInt64(10);
      invite.updated_at = row.getInt64(11);
      invite.expires_at = row.getInt64(12);
      return invite;
    }

    WorkspaceInviteResult<std::string> find_user_id_by_email(const std::string &email) const
    {
      auto rows = db->query(
          "SELECT id FROM users WHERE email = ? AND active = 1 "
          "UNION "
          "SELECT id FROM rix_auth_users WHERE email = ? AND active = 1 "
          "LIMIT 1",
          email,
          email);

      if (!rows->next())
      {
        return WorkspaceInviteResult<std::string>::failure({support::WorkspaceInviteErrorCode::UserNotFound,
                                                            "User not found for this email"});
      }

      return WorkspaceInviteResult<std::string>::success(rows->row().getString(0));
    }

    bool project_ids_belong_to_workspace(const std::string &workspace_id, const std::string &project_ids_json) const
    {
      if (selected_projects_empty(project_ids_json))
      {
        return false;
      }

      auto rows = db->query("SELECT id FROM projects WHERE workspace_id = ?", workspace_id);
      std::string known;
      while (rows->next())
      {
        known += "\"" + rows->row().getString(0) + "\"";
      }

      std::size_t pos = 0;
      while ((pos = project_ids_json.find("project_", pos)) != std::string::npos)
      {
        auto end = project_ids_json.find('"', pos);
        if (end == std::string::npos) end = project_ids_json.find(',', pos);
        const auto project_id = project_ids_json.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
        if (known.find("\"" + project_id + "\"") == std::string::npos)
        {
          return false;
        }
        pos += project_id.size();
      }

      return true;
    }

    bool active_member_exists(const std::string &workspace_id, const std::string &user_id) const
    {
      auto rows = db->query(
          "SELECT id FROM workspace_members WHERE workspace_id = ? AND user_id = ? AND active = 1 LIMIT 1",
          workspace_id,
          user_id);
      return rows->next();
    }

    bool pending_invite_exists(const std::string &workspace_id, const std::string &email) const
    {
      auto rows = db->query(
          "SELECT id FROM workspace_invites WHERE workspace_id = ? AND invited_email = ? AND status = 'pending' LIMIT 1",
          workspace_id,
          email);
      return rows->next();
    }

    WorkspaceInviteResult<dto::WorkspaceInviteResponse> find_invite(const std::string &invite_id) const
    {
      auto rows = db->query(
          "SELECT wi.id, wi.workspace_id, COALESCE(w.name, ''), wi.invited_email, COALESCE(wi.invited_user_id, ''), "
          "wi.role, wi.invited_by_user_id, wi.status, COALESCE(wi.access_scope, 'entire_workspace'), COALESCE(wi.project_ids_json, ''), wi.created_at, wi.updated_at, COALESCE(wi.expires_at, 0) "
          "FROM workspace_invites wi LEFT JOIN workspaces w ON w.id = wi.workspace_id WHERE wi.id = ? LIMIT 1",
          invite_id);

      if (!rows->next())
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                             "Workspace invite not found."});
      }

      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(row_to_invite(rows->row()));
    }

    void ensure_member_from_invite(const dto::WorkspaceInviteResponse &invite, const std::string &user_id)
    {
      const auto timestamp = now_timestamp();
      db->exec(
          "INSERT INTO workspace_members "
          "(id, workspace_id, user_id, email, role, status, active, access_scope, project_ids_json, invited_by_user_id, joined_at, created_at, updated_at) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          "member_" + invite.id,
          invite.workspace_id,
          user_id,
          invite.invited_email,
          invite.role,
          "active",
          static_cast<std::int64_t>(1),
          invite.access_scope,
          invite.project_ids_json,
          invite.invited_by_user_id,
          timestamp,
          timestamp,
          timestamp);
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::WorkspaceInviteResponse> invites;
  };

  WorkspaceInviteService::WorkspaceInviteService()
      : impl_(std::make_unique<Impl>()) {}
  WorkspaceInviteService::~WorkspaceInviteService() = default;
  WorkspaceInviteService::WorkspaceInviteService(WorkspaceInviteService &&) noexcept = default;
  WorkspaceInviteService &WorkspaceInviteService::operator=(WorkspaceInviteService &&) noexcept = default;

  WorkspaceInviteResult<dto::WorkspaceInviteResponse> WorkspaceInviteService::create_invite(const dto::CreateWorkspaceInviteRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingWorkspace,
                                                                           "Workspace is required."});
    }
    if (request.invited_email.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingEmail,
                                                                           "Email is required."});
    }
    if (request.invited_by_user_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingActor,
                                                                           "Actor is required."});
    }

    const auto role = normalize_role(request.role);
    if (!valid_role(role))
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::InvalidRole,
                                                                           "Member role must be admin, member or viewer."});
    }

    const auto access_scope = normalize_access_scope(request.access_scope);
    if (!valid_access_scope(access_scope))
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::InvalidRequest,
                                                                           "Access scope must be entire_workspace or selected_projects."});
    }
    if (access_scope == "selected_projects" && selected_projects_empty(request.project_ids_json))
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::InvalidRequest,
                                                                           "Selected projects access requires at least one project."});
    }

    const auto timestamp = now_timestamp();

    if (impl_->persistent())
    {
      auto user = impl_->find_user_id_by_email(request.invited_email);
      if (user.failed())
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure(user.error());
      }

      if (impl_->active_member_exists(request.workspace_id, user.value()))
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceMemberAlreadyExists,
                                                                             "Member already exists in this workspace."});
      }

      if (access_scope == "selected_projects" && !impl_->project_ids_belong_to_workspace(request.workspace_id, request.project_ids_json))
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::InvalidRequest,
                                                                             "Selected projects must belong to this workspace."});
      }

      if (impl_->pending_invite_exists(request.workspace_id, request.invited_email))
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteAlreadyExists,
                                                                             "Workspace invite already exists."});
      }

      dto::WorkspaceInviteResponse invite;
      invite.id = make_invite_id();
      invite.workspace_id = request.workspace_id;
      invite.invited_email = request.invited_email;
      invite.invited_user_id = user.value();
      invite.role = role;
      invite.invited_by_user_id = request.invited_by_user_id;
      invite.status = "pending";
      invite.access_scope = access_scope;
      invite.project_ids_json = request.project_ids_json;
      invite.created_at = timestamp;
      invite.updated_at = timestamp;
      invite.expires_at = request.expires_at;

      impl_->db->exec(
          "INSERT INTO workspace_invites "
          "(id, workspace_id, invited_email, invited_user_id, role, invited_by_user_id, status, access_scope, project_ids_json, created_at, updated_at, expires_at) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          invite.id,
          invite.workspace_id,
          invite.invited_email,
          invite.invited_user_id,
          invite.role,
          invite.invited_by_user_id,
          invite.status,
          invite.access_scope,
          invite.project_ids_json,
          invite.created_at,
          invite.updated_at,
          invite.expires_at);

      auto created = impl_->find_invite(invite.id);
      return created.ok() ? created : WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(invite);
    }

    dto::WorkspaceInviteResponse invite;
    invite.id = make_invite_id();
    invite.workspace_id = request.workspace_id;
    invite.invited_email = request.invited_email;
    invite.role = role;
    invite.invited_by_user_id = request.invited_by_user_id;
    invite.status = "pending";
    invite.access_scope = access_scope;
    invite.project_ids_json = request.project_ids_json;
    invite.created_at = timestamp;
    invite.updated_at = timestamp;
    invite.expires_at = request.expires_at;
    impl_->invites[invite.id] = invite;
    return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(invite);
  }

  WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>> WorkspaceInviteService::list_invites(const dto::ListWorkspaceInvitesRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>>::failure({support::WorkspaceInviteErrorCode::MissingWorkspace,
                                                                                        "Workspace is required."});
    }

    std::vector<dto::WorkspaceInviteResponse> invites;
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT wi.id, wi.workspace_id, COALESCE(w.name, ''), wi.invited_email, COALESCE(wi.invited_user_id, ''), "
          "wi.role, wi.invited_by_user_id, wi.status, COALESCE(wi.access_scope, 'entire_workspace'), COALESCE(wi.project_ids_json, ''), wi.created_at, wi.updated_at, COALESCE(wi.expires_at, 0) "
          "FROM workspace_invites wi LEFT JOIN workspaces w ON w.id = wi.workspace_id WHERE wi.workspace_id = ? ORDER BY wi.created_at DESC",
          request.workspace_id);
      while (rows->next())
      {
        invites.push_back(impl_->row_to_invite(rows->row()));
      }
      return WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>>::success(invites);
    }

    for (const auto &entry : impl_->invites)
    {
      if (entry.second.workspace_id == request.workspace_id)
      {
        invites.push_back(entry.second);
      }
    }
    return WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>>::success(invites);
  }

  WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>> WorkspaceInviteService::list_my_invites(const dto::ListMyWorkspaceInvitesRequest &request) const
  {
    if (request.user_id.empty() && request.email.empty())
    {
      return WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>>::failure({support::WorkspaceInviteErrorCode::MissingActor,
                                                                                        "Actor is required."});
    }

    std::vector<dto::WorkspaceInviteResponse> invites;
    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT wi.id, wi.workspace_id, COALESCE(w.name, ''), wi.invited_email, COALESCE(wi.invited_user_id, ''), "
          "wi.role, wi.invited_by_user_id, wi.status, COALESCE(wi.access_scope, 'entire_workspace'), COALESCE(wi.project_ids_json, ''), wi.created_at, wi.updated_at, COALESCE(wi.expires_at, 0) "
          "FROM workspace_invites wi LEFT JOIN workspaces w ON w.id = wi.workspace_id "
          "WHERE wi.status = 'pending' AND (wi.invited_user_id = ? OR wi.invited_email = ?) ORDER BY wi.created_at DESC",
          request.user_id,
          request.email);
      while (rows->next())
      {
        invites.push_back(impl_->row_to_invite(rows->row()));
      }
      return WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>>::success(invites);
    }

    for (const auto &entry : impl_->invites)
    {
      if (entry.second.status == "pending" &&
          (entry.second.invited_user_id == request.user_id || entry.second.invited_email == request.email))
      {
        invites.push_back(entry.second);
      }
    }
    return WorkspaceInviteResult<std::vector<dto::WorkspaceInviteResponse>>::success(invites);
  }

  WorkspaceInviteResult<dto::WorkspaceInviteResponse> WorkspaceInviteService::accept_invite(const dto::WorkspaceInviteActionRequest &request)
  {
    if (request.invite_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingInvite,
                                                                           "Invite is required."});
    }

    if (request.user_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingActor,
                                                                           "Actor is required."});
    }

    if (impl_->persistent())
    {
      auto invite_result = impl_->find_invite(request.invite_id);
      if (invite_result.failed())
      {
        return invite_result;
      }

      auto invite = invite_result.value();
      if (invite.status != "pending")
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotPending,
                                                                             "Workspace invite is not pending."});
      }

      if (invite.invited_user_id != request.user_id && invite.invited_email != request.email)
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                             "Workspace invite not found."});
      }

      if (impl_->active_member_exists(invite.workspace_id, request.user_id))
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceMemberAlreadyExists,
                                                                             "Member already exists in this workspace."});
      }

      impl_->ensure_member_from_invite(invite, request.user_id);
      invite.status = "accepted";
      invite.updated_at = now_timestamp();
      invite.invited_user_id = request.user_id;
      impl_->db->exec(
          "UPDATE workspace_invites SET status = ?, invited_user_id = ?, updated_at = ? WHERE id = ?",
          invite.status,
          invite.invited_user_id,
          invite.updated_at,
          invite.id);
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(invite);
    }

    auto it = impl_->invites.find(request.invite_id);
    if (it == impl_->invites.end())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                           "Workspace invite not found."});
    }
    it->second.status = "accepted";
    it->second.updated_at = now_timestamp();
    return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(it->second);
  }

  WorkspaceInviteResult<dto::WorkspaceInviteResponse> WorkspaceInviteService::decline_invite(const dto::WorkspaceInviteActionRequest &request)
  {
    if (request.invite_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingInvite,
                                                                           "Invite is required."});
    }

    if (impl_->persistent())
    {
      auto invite_result = impl_->find_invite(request.invite_id);
      if (invite_result.failed())
      {
        return invite_result;
      }
      auto invite = invite_result.value();
      if (invite.status != "pending")
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotPending,
                                                                             "Workspace invite is not pending."});
      }
      if (invite.invited_user_id != request.user_id && invite.invited_email != request.email)
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                             "Workspace invite not found."});
      }
      invite.status = "declined";
      invite.updated_at = now_timestamp();
      impl_->db->exec("UPDATE workspace_invites SET status = ?, updated_at = ? WHERE id = ?", invite.status, invite.updated_at, invite.id);
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(invite);
    }

    auto it = impl_->invites.find(request.invite_id);
    if (it == impl_->invites.end())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                           "Workspace invite not found."});
    }
    it->second.status = "declined";
    it->second.updated_at = now_timestamp();
    return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(it->second);
  }

  WorkspaceInviteResult<dto::WorkspaceInviteResponse> WorkspaceInviteService::revoke_invite(const dto::RevokeWorkspaceInviteRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingWorkspace,
                                                                           "Workspace is required."});
    }
    if (request.invite_id.empty())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::MissingInvite,
                                                                           "Invite is required."});
    }

    if (impl_->persistent())
    {
      auto invite_result = impl_->find_invite(request.invite_id);
      if (invite_result.failed())
      {
        return invite_result;
      }
      auto invite = invite_result.value();
      if (invite.workspace_id != request.workspace_id)
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                             "Workspace invite not found."});
      }
      if (invite.status != "pending")
      {
        return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotPending,
                                                                             "Workspace invite is not pending."});
      }
      invite.status = "revoked";
      invite.updated_at = now_timestamp();
      impl_->db->exec("UPDATE workspace_invites SET status = ?, updated_at = ? WHERE id = ?", invite.status, invite.updated_at, invite.id);
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(invite);
    }

    auto it = impl_->invites.find(request.invite_id);
    if (it == impl_->invites.end())
    {
      return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::failure({support::WorkspaceInviteErrorCode::WorkspaceInviteNotFound,
                                                                           "Workspace invite not found."});
    }
    it->second.status = "revoked";
    it->second.updated_at = now_timestamp();
    return WorkspaceInviteResult<dto::WorkspaceInviteResponse>::success(it->second);
  }
}
