/**
 *
 *  @file MemberService.hpp
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
#ifndef cloud_members_services_member_service_hpp
#define cloud_members_services_member_service_hpp

#include <members/dto/MemberRequests.hpp>
#include <members/dto/MemberResponses.hpp>
#include <members/support/MemberErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::members::services
{
  template <typename T>
  class MemberResult
  {
  public:
    static MemberResult success(T value)
    {
      MemberResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static MemberResult failure(
        support::MemberError error)
    {
      MemberResult result;
      result.ok_ = false;
      result.error_ = std::move(error);
      return result;
    }

    bool ok() const
    {
      return ok_;
    }

    bool failed() const
    {
      return !ok_;
    }

    const T &value() const
    {
      return value_;
    }

    const support::MemberError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::MemberError error_{};
  };

  class MemberService
  {
  public:
    MemberService();
    ~MemberService();

    MemberService(const MemberService &) = delete;
    MemberService &operator=(const MemberService &) = delete;

    MemberService(MemberService &&) noexcept;
    MemberService &operator=(MemberService &&) noexcept;

    MemberResult<dto::MemberResponse> invite_member(
        const dto::InviteMemberRequest &request);

    MemberResult<dto::MemberResponse> update_member_role(
        const dto::UpdateMemberRoleRequest &request);

    MemberResult<dto::MemberResponse> remove_member(
        const dto::RemoveMemberRequest &request);

    MemberResult<std::vector<dto::MemberResponse>> list_members(
        const dto::ListMembersRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::members::services

#endif // cloud_members_services_member_service_hpp
