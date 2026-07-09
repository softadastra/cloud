/**
 *
 *  @file test_tokens.cpp
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
#include <tokens/TokensModule.hpp>
#include <tokens/services/TokenService.hpp>

#include <string>
#include <vector>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("tokens module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::tokens::TokensModule::name()),
                              std::string("tokens")); }));

  registry.add(TestCase("token service creates a valid token", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.name = "CLI token";
                          request.scopes = {
                              "registry:read",
                              "registry:write",
                              "projects:read"};

                          auto created = service.create_token(request);

                          Assert::equal(created.ok(), true);
                          Assert::equal(
                              created.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              created.value().user_id,
                              std::string("user_123"));
                          Assert::equal(
                              created.value().name,
                              std::string("CLI token"));
                          Assert::equal(
                              created.value().revoked,
                              false);
                          Assert::equal(
                              static_cast<int>(created.value().scopes.size()),
                              3); }));

  registry.add(TestCase("token service rejects empty scopes", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.name = "CLI token";

                          auto created = service.create_token(request);

                          Assert::equal(created.failed(), true); }));

  registry.add(TestCase("token service rejects invalid scope", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.name = "CLI token";
                          request.scopes = {
                              "registry:read",
                              "invalid:scope"};

                          auto created = service.create_token(request);

                          Assert::equal(created.failed(), true); }));

  registry.add(TestCase("token service lists tokens by workspace and user", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest first;
                          first.workspace_id = "workspace_123";
                          first.user_id = "user_123";
                          first.name = "Read token";
                          first.scopes = {
                              "registry:read"};

                          cloud::tokens::dto::CreateTokenRequest second;
                          second.workspace_id = "workspace_123";
                          second.user_id = "user_123";
                          second.name = "Write token";
                          second.scopes = {
                              "registry:write"};

                          cloud::tokens::dto::CreateTokenRequest third;
                          third.workspace_id = "workspace_456";
                          third.user_id = "user_123";
                          third.name = "Other token";
                          third.scopes = {
                              "projects:read"};

                          service.create_token(first);
                          service.create_token(second);
                          service.create_token(third);

                          cloud::tokens::dto::ListTokensRequest list;
                          list.workspace_id = "workspace_123";
                          list.user_id = "user_123";

                          auto tokens = service.list_tokens(list);

                          Assert::equal(tokens.ok(), true);
                          Assert::equal(
                              static_cast<int>(tokens.value().size()),
                              2); }));

  registry.add(TestCase("token service finds token by id", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.name = "CLI token";
                          request.scopes = {
                              "registry:read"};

                          auto created = service.create_token(request);

                          cloud::tokens::dto::TokenLookupRequest lookup;
                          lookup.token_id = created.value().id;
                          lookup.workspace_id = "workspace_123";
                          lookup.user_id = "user_123";

                          auto found = service.find_token(lookup);

                          Assert::equal(found.ok(), true);
                          Assert::equal(
                              found.value().id,
                              created.value().id); }));

  registry.add(TestCase("token service revokes token", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.name = "CLI token";
                          request.scopes = {
                              "registry:read"};

                          auto created = service.create_token(request);

                          cloud::tokens::dto::RevokeTokenRequest revoke;
                          revoke.token_id = created.value().id;
                          revoke.workspace_id = "workspace_123";
                          revoke.user_id = "user_123";

                          auto revoked = service.revoke_token(revoke);

                          Assert::equal(revoked.ok(), true);
                          Assert::equal(
                              revoked.value().revoked,
                              true); }));

  registry.add(TestCase("token service rejects double revoke", []
                        {
                          cloud::tokens::services::TokenService service;

                          cloud::tokens::dto::CreateTokenRequest request;
                          request.workspace_id = "workspace_123";
                          request.user_id = "user_123";
                          request.name = "CLI token";
                          request.scopes = {
                              "registry:read"};

                          auto created = service.create_token(request);

                          cloud::tokens::dto::RevokeTokenRequest revoke;
                          revoke.token_id = created.value().id;
                          revoke.workspace_id = "workspace_123";
                          revoke.user_id = "user_123";

                          auto first = service.revoke_token(revoke);
                          auto second = service.revoke_token(revoke);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true); }));

  return TestRunner::run_all_and_exit();
}
