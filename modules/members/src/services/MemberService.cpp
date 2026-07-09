/**
 *
 *  @file MemberService.cpp
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
#include <members/services/MemberService.hpp>

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

namespace cloud::members::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_member_id()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "member_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    bool valid_role(
        const std::string &role)
    {
      return role == "owner" ||
             role == "admin" ||
             role == "member" ||
             role == "viewer";
    }

    std::string normalize_role(
        const std::string &role)
    {
      if (role.empty())
      {
        return "member";
      }

      return role;
    }

    std::string member_key(
        const std::string &workspace_id,
        const std::string &user_id)
    {
      return workspace_id + "::" + user_id;
    }
  } // namespace

  class MemberService::Impl
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

    bool persistent() const
    {
      return db != nullptr;
    }

    dto::MemberResponse row_to_member(
        const vix::db::ResultRow &row) const
    {
      dto::MemberResponse member;
      member.id = row.getString(0);
      member.workspace_id = row.getString(1);
      member.user_id = row.getString(2);
      member.email = row.getString(3);
      member.role = row.getString(4);
      member.status = row.getString(5);
      member.invited_by_user_id = row.getString(6);
      member.created_at = row.getInt64(7);
      member.updated_at = row.getInt64(8);
      return member;
    }

    bool workspace_exists(
        const std::string &workspace_id) const
    {
      auto rows = db->query(
          "SELECT id FROM workspaces WHERE id = ? LIMIT 1",
          workspace_id);

      return rows->next();
    }

    MemberResult<dto::MemberResponse> find_member(
        const std::string &workspace_id,
        const std::string &user_id,
        bool active_only) const
    {
      const auto sql = active_only
                           ? "SELECT id, workspace_id, user_id, email, role, status, invited_by_user_id, created_at, updated_at FROM workspace_members WHERE workspace_id = ? AND user_id = ? AND active = 1 LIMIT 1"
                           : "SELECT id, workspace_id, user_id, email, role, status, invited_by_user_id, created_at, updated_at FROM workspace_members WHERE workspace_id = ? AND user_id = ? LIMIT 1";

      auto rows = db->query(sql, workspace_id, user_id);

      if (!rows->next())
      {
        return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberNotFound,
                                                           "Member not found."});
      }

      return MemberResult<dto::MemberResponse>::success(row_to_member(rows->row()));
    }

    std::unique_ptr<vix::db::Database> db;
    std::unordered_map<std::string, dto::MemberResponse> members_by_id;
    std::unordered_map<std::string, std::string> member_id_by_workspace_user;
  };

  MemberService::MemberService()
      : impl_(std::make_unique<Impl>())
  {
  }

  MemberService::~MemberService() = default;

  MemberService::MemberService(MemberService &&) noexcept = default;

  MemberService &MemberService::operator=(MemberService &&) noexcept = default;

  MemberResult<dto::MemberResponse> MemberService::invite_member(
      const dto::InviteMemberRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingWorkspace,
                                                         "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingUser,
                                                         "User is required."});
    }

    if (request.email.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingEmail,
                                                         "Email is required."});
    }

    const auto role = normalize_role(request.role);

    if (!valid_role(role))
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::InvalidRole,
                                                         "Member role must be owner, admin, member or viewer."});
    }

    if (impl_->persistent())
    {
      if (!impl_->workspace_exists(request.workspace_id))
      {
        return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingWorkspace,
                                                           "Workspace not found."});
      }

      auto existing = impl_->find_member(request.workspace_id, request.user_id, false);

      if (existing.ok() && existing.value().status == "active")
      {
        return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberAlreadyExists,
                                                           "Member already exists in this workspace."});
      }

      const auto timestamp = now_timestamp();

      if (existing.ok())
      {
        auto member = existing.value();
        member.email = request.email;
        member.role = role;
        member.status = "active";
        member.invited_by_user_id = request.invited_by_user_id;
        member.updated_at = timestamp;

        impl_->db->exec(
            "UPDATE workspace_members "
            "SET email = ?, role = ?, status = ?, active = 1, invited_by_user_id = ?, updated_at = ? "
            "WHERE workspace_id = ? AND user_id = ?",
            member.email,
            member.role,
            member.status,
            member.invited_by_user_id,
            member.updated_at,
            member.workspace_id,
            member.user_id);

        return MemberResult<dto::MemberResponse>::success(member);
      }

      dto::MemberResponse member;
      member.id = make_member_id();
      member.workspace_id = request.workspace_id;
      member.user_id = request.user_id;
      member.email = request.email;
      member.role = role;
      member.status = "active";
      member.invited_by_user_id = request.invited_by_user_id;
      member.created_at = timestamp;
      member.updated_at = timestamp;

      impl_->db->exec(
          "INSERT INTO workspace_members "
          "(id, workspace_id, user_id, email, role, status, active, invited_by_user_id, joined_at, created_at, updated_at) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          member.id,
          member.workspace_id,
          member.user_id,
          member.email,
          member.role,
          member.status,
          static_cast<std::int64_t>(1),
          member.invited_by_user_id,
          member.created_at,
          member.created_at,
          member.updated_at);

      return MemberResult<dto::MemberResponse>::success(member);
    }

    const auto key = member_key(request.workspace_id, request.user_id);

    if (impl_->member_id_by_workspace_user.find(key) !=
        impl_->member_id_by_workspace_user.end())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberAlreadyExists,
                                                         "Member already exists in this workspace."});
    }

    const auto timestamp = now_timestamp();

    dto::MemberResponse member;
    member.id = make_member_id();
    member.workspace_id = request.workspace_id;
    member.user_id = request.user_id;
    member.email = request.email;
    member.role = role;
    member.status = "active";
    member.invited_by_user_id = request.invited_by_user_id;
    member.created_at = timestamp;
    member.updated_at = timestamp;

    impl_->member_id_by_workspace_user[key] = member.id;
    impl_->members_by_id[member.id] = member;

    return MemberResult<dto::MemberResponse>::success(member);
  }

  MemberResult<dto::MemberResponse> MemberService::update_member_role(
      const dto::UpdateMemberRoleRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingWorkspace,
                                                         "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingUser,
                                                         "User is required."});
    }

    const auto role = normalize_role(request.role);

    if (!valid_role(role))
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::InvalidRole,
                                                         "Member role must be owner, admin, member or viewer."});
    }

    if (impl_->persistent())
    {
      auto current = impl_->find_member(request.workspace_id, request.user_id, true);

      if (current.failed())
      {
        return current;
      }

      auto member = current.value();
      member.role = role;
      member.updated_at = now_timestamp();

      impl_->db->exec(
          "UPDATE workspace_members SET role = ?, updated_at = ? WHERE workspace_id = ? AND user_id = ? AND active = 1",
          member.role,
          member.updated_at,
          member.workspace_id,
          member.user_id);

      return MemberResult<dto::MemberResponse>::success(member);
    }

    const auto key = member_key(request.workspace_id, request.user_id);
    auto member_id = impl_->member_id_by_workspace_user.find(key);

    if (member_id == impl_->member_id_by_workspace_user.end())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberNotFound,
                                                         "Member not found."});
    }

    auto member = impl_->members_by_id.find(member_id->second);

    if (member == impl_->members_by_id.end())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberNotFound,
                                                         "Member not found."});
    }

    member->second.role = role;
    member->second.updated_at = now_timestamp();

    return MemberResult<dto::MemberResponse>::success(member->second);
  }

  MemberResult<dto::MemberResponse> MemberService::remove_member(
      const dto::RemoveMemberRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingWorkspace,
                                                         "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MissingUser,
                                                         "User is required."});
    }

    if (impl_->persistent())
    {
      auto current = impl_->find_member(request.workspace_id, request.user_id, true);

      if (current.failed())
      {
        return current;
      }

      auto removed = current.value();
      removed.status = "removed";
      removed.updated_at = now_timestamp();

      impl_->db->exec(
          "UPDATE workspace_members SET status = ?, active = 0, updated_at = ? WHERE workspace_id = ? AND user_id = ? AND active = 1",
          removed.status,
          removed.updated_at,
          removed.workspace_id,
          removed.user_id);

      return MemberResult<dto::MemberResponse>::success(removed);
    }

    const auto key = member_key(request.workspace_id, request.user_id);
    auto member_id = impl_->member_id_by_workspace_user.find(key);

    if (member_id == impl_->member_id_by_workspace_user.end())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberNotFound,
                                                         "Member not found."});
    }

    auto member = impl_->members_by_id.find(member_id->second);

    if (member == impl_->members_by_id.end())
    {
      return MemberResult<dto::MemberResponse>::failure({support::MemberErrorCode::MemberNotFound,
                                                         "Member not found."});
    }

    auto removed = member->second;
    removed.status = "removed";
    removed.updated_at = now_timestamp();

    impl_->members_by_id.erase(member);
    impl_->member_id_by_workspace_user.erase(member_id);

    return MemberResult<dto::MemberResponse>::success(removed);
  }

  MemberResult<std::vector<dto::MemberResponse>> MemberService::list_members(
      const dto::ListMembersRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return MemberResult<std::vector<dto::MemberResponse>>::failure({support::MemberErrorCode::MissingWorkspace,
                                                                      "Workspace is required."});
    }

    std::vector<dto::MemberResponse> members;

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, user_id, email, role, status, invited_by_user_id, created_at, updated_at "
          "FROM workspace_members WHERE workspace_id = ? AND active = 1 ORDER BY created_at",
          request.workspace_id);

      while (rows->next())
      {
        members.push_back(impl_->row_to_member(rows->row()));
      }

      return MemberResult<std::vector<dto::MemberResponse>>::success(members);
    }

    for (const auto &entry : impl_->members_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id)
      {
        members.push_back(entry.second);
      }
    }

    std::sort(
        members.begin(),
        members.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return MemberResult<std::vector<dto::MemberResponse>>::success(members);
  }
} // namespace cloud::members::services
