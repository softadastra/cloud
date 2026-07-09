#include <workspace_invites/WorkspaceInvitesModule.hpp>
#include <workspace_invites/support/WorkspaceInviteErrors.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("workspace_invites module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::workspace_invites::WorkspaceInvitesModule::name()),
                              std::string("workspace_invites")); }));

  registry.add(TestCase("workspace invite duplicate exposes public contract", []
                        {
                          cloud::workspace_invites::support::WorkspaceInviteError error{
                              cloud::workspace_invites::support::WorkspaceInviteErrorCode::WorkspaceInviteAlreadyExists,
                              "Workspace invite already exists."};

                          Assert::equal(
                              cloud::workspace_invites::support::http_status_for_workspace_invite_error(error),
                              409);
                          Assert::equal(
                              cloud::workspace_invites::support::public_code_for_workspace_invite_error(error),
                              std::string("workspace_invite_already_exists")); }));

  registry.add(TestCase("workspace invite user not found exposes public contract", []
                        {
                          cloud::workspace_invites::support::WorkspaceInviteError error{
                              cloud::workspace_invites::support::WorkspaceInviteErrorCode::UserNotFound,
                              "User not found for this email"};

                          Assert::equal(
                              cloud::workspace_invites::support::http_status_for_workspace_invite_error(error),
                              404);
                          Assert::equal(
                              cloud::workspace_invites::support::public_code_for_workspace_invite_error(error),
                              std::string("user_not_found")); }));

  registry.add(TestCase("workspace invite existing member exposes public contract", []
                        {
                          cloud::workspace_invites::support::WorkspaceInviteError error{
                              cloud::workspace_invites::support::WorkspaceInviteErrorCode::WorkspaceMemberAlreadyExists,
                              "Member already exists in this workspace."};

                          Assert::equal(
                              cloud::workspace_invites::support::http_status_for_workspace_invite_error(error),
                              409);
                          Assert::equal(
                              cloud::workspace_invites::support::public_code_for_workspace_invite_error(error),
                              std::string("workspace_member_already_exists")); }));

  return TestRunner::run_all_and_exit();
}
