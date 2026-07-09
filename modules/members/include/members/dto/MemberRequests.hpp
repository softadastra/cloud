/**
 *
 *  @file MemberRequests.hpp
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
#ifndef cloud_members_dto_member_requests_hpp
#define cloud_members_dto_member_requests_hpp

#include <string>

namespace cloud::members::dto
{
  struct InviteMemberRequest
  {
    std::string workspace_id;
    std::string user_id;
    std::string email;
    std::string role;
    std::string invited_by_user_id;
  };

  struct UpdateMemberRoleRequest
  {
    std::string workspace_id;
    std::string user_id;
    std::string role;
  };

  struct RemoveMemberRequest
  {
    std::string workspace_id;
    std::string user_id;
  };

  struct ListMembersRequest
  {
    std::string workspace_id;
  };
} // namespace cloud::members::dto

#endif // cloud_members_dto_member_requests_hpp
