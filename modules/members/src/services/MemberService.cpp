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
#include <sstream>
#include <string>
#include <unordered_map>

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
