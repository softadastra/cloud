/**
 *
 *  @file test_members.cpp
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
#include <members/MembersModule.hpp>
#include <members/services/MemberService.hpp>
#include <members/support/MemberErrors.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("members module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::members::MembersModule::name()),
                              std::string("members")); }));

  registry.add(TestCase("member service invites a valid member", []
                        {
                          cloud::members::services::MemberService service;

                          cloud::members::dto::InviteMemberRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.email = "gaspard@softadastra.com";
                          request.role = "admin";
                          request.invited_by_user_id = "owner_123";

                          auto invited = service.invite_member(request);

                          Assert::equal(invited.ok(), true);
                          Assert::equal(
                              invited.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              invited.value().user_id,
                              std::string("user_123"));
                          Assert::equal(
                              invited.value().role,
                              std::string("admin"));
                          Assert::equal(
                              invited.value().status,
                              std::string("active")); }));

  registry.add(TestCase("member service uses member role by default", []
                        {
                          cloud::members::services::MemberService service;

                          cloud::members::dto::InviteMemberRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.email = "gaspard@softadastra.com";
                          request.invited_by_user_id = "owner_123";

                          auto invited = service.invite_member(request);

                          Assert::equal(invited.ok(), true);
                          Assert::equal(
                              invited.value().role,
                              std::string("member")); }));

  registry.add(TestCase("member service rejects duplicate member", []
                        {
                          cloud::members::services::MemberService service;

                          cloud::members::dto::InviteMemberRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.email = "gaspard@softadastra.com";
                          request.role = "member";
                          request.invited_by_user_id = "owner_123";

                          auto first = service.invite_member(request);
                          auto second = service.invite_member(request);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true);
                          Assert::equal(
                              cloud::members::support::http_status_for_member_error(second.error()),
                              409);
                          Assert::equal(
                              cloud::members::support::public_code_for_member_error(second.error()),
                              std::string("workspace_member_already_exists")); }));

  registry.add(TestCase("member errors expose user not found contract", []
                        {
                          cloud::members::support::MemberError error{
                              cloud::members::support::MemberErrorCode::UserNotFound,
                              "User not found for this email"};

                          Assert::equal(
                              cloud::members::support::http_status_for_member_error(error),
                              404);
                          Assert::equal(
                              cloud::members::support::public_code_for_member_error(error),
                              std::string("user_not_found"));
                          Assert::equal(
                              cloud::members::support::public_message_for_member_error(error),
                              std::string("User not found for this email")); }));

  registry.add(TestCase("member service updates member role", []
                        {
                          cloud::members::services::MemberService service;

                          cloud::members::dto::InviteMemberRequest invite;
                          invite.workspace_id = "workspace_123";
                          invite.user_id = "user_123";
                          invite.email = "gaspard@softadastra.com";
                          invite.role = "member";
                          invite.invited_by_user_id = "owner_123";

                          service.invite_member(invite);

                          cloud::members::dto::UpdateMemberRoleRequest update;
                          update.workspace_id = "workspace_123";
                          update.user_id = "user_123";
                          update.role = "viewer";

                          auto updated = service.update_member_role(update);

                          Assert::equal(updated.ok(), true);
                          Assert::equal(
                              updated.value().role,
                              std::string("viewer")); }));

  registry.add(TestCase("member service lists members by workspace", []
                        {
                          cloud::members::services::MemberService service;

                          cloud::members::dto::InviteMemberRequest first;
                          first.workspace_id = "workspace_123";
                          first.user_id = "user_123";
                          first.email = "first@softadastra.com";
                          first.role = "admin";
                          first.invited_by_user_id = "owner_123";

                          cloud::members::dto::InviteMemberRequest second;
                          second.workspace_id = "workspace_123";
                          second.user_id = "user_456";
                          second.email = "second@softadastra.com";
                          second.role = "member";
                          second.invited_by_user_id = "owner_123";

                          cloud::members::dto::InviteMemberRequest third;
                          third.workspace_id = "workspace_456";
                          third.user_id = "user_789";
                          third.email = "third@softadastra.com";
                          third.role = "viewer";
                          third.invited_by_user_id = "owner_456";

                          service.invite_member(first);
                          service.invite_member(second);
                          service.invite_member(third);

                          cloud::members::dto::ListMembersRequest list;
                          list.workspace_id = "workspace_123";

                          auto members = service.list_members(list);

                          Assert::equal(members.ok(), true);
                          Assert::equal(
                              static_cast<int>(members.value().size()),
                              2); }));

  registry.add(TestCase("member service removes member", []
                        {
                          cloud::members::services::MemberService service;

                          cloud::members::dto::InviteMemberRequest invite;
                          invite.workspace_id = "workspace_123";
                          invite.user_id = "user_123";
                          invite.email = "gaspard@softadastra.com";
                          invite.role = "member";
                          invite.invited_by_user_id = "owner_123";

                          service.invite_member(invite);

                          cloud::members::dto::RemoveMemberRequest remove;
                          remove.workspace_id = "workspace_123";
                          remove.user_id = "user_123";

                          auto removed = service.remove_member(remove);

                          Assert::equal(removed.ok(), true);
                          Assert::equal(
                              removed.value().status,
                              std::string("removed"));

                          cloud::members::dto::ListMembersRequest list;
                          list.workspace_id = "workspace_123";

                          auto members = service.list_members(list);

                          Assert::equal(members.ok(), true);
                          Assert::equal(
                              static_cast<int>(members.value().size()),
                              0); }));

  return TestRunner::run_all_and_exit();
}
