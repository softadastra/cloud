/**
 *
 *  @file test_lockfiles.cpp
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
#include <lockfiles/LockfilesModule.hpp>
#include <lockfiles/services/LockfileService.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("lockfiles module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::lockfiles::LockfilesModule::name()),
                              std::string("lockfiles")); }));

  registry.add(TestCase("lockfile service uploads a valid lockfile", []
                        {
                          cloud::lockfiles::services::LockfileService service;

                          cloud::lockfiles::dto::UploadLockfileRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.uploaded_by_user_id = "user_123";
                          request.lockfile_json = "{\"packages\":[{\"name\":\"vix/http\",\"version\":\"1.0.0\"}]}";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          request.source = "vix";

                          auto uploaded = service.upload_lockfile(request);

                          Assert::equal(uploaded.ok(), true);
                          Assert::equal(
                              uploaded.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              uploaded.value().project_id,
                              std::string("project_123"));
                          Assert::equal(
                              uploaded.value().uploaded_by_user_id,
                              std::string("user_123"));
                          Assert::equal(
                              uploaded.value().source,
                              std::string("vix")); }));

  registry.add(TestCase("lockfile service uses vix as default source", []
                        {
                          cloud::lockfiles::services::LockfileService service;

                          cloud::lockfiles::dto::UploadLockfileRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.uploaded_by_user_id = "user_123";
                          request.lockfile_json = "{\"packages\":[]}";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

                          auto uploaded = service.upload_lockfile(request);

                          Assert::equal(uploaded.ok(), true);
                          Assert::equal(
                              uploaded.value().source,
                              std::string("vix")); }));

  registry.add(TestCase("lockfile service rejects invalid checksum", []
                        {
                          cloud::lockfiles::services::LockfileService service;

                          cloud::lockfiles::dto::UploadLockfileRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.uploaded_by_user_id = "user_123";
                          request.lockfile_json = "{\"packages\":[]}";
                          request.checksum_sha256 = "invalid";

                          auto uploaded = service.upload_lockfile(request);

                          Assert::equal(uploaded.failed(), true); }));

  registry.add(TestCase("lockfile service rejects missing lockfile content", []
                        {
                          cloud::lockfiles::services::LockfileService service;

                          cloud::lockfiles::dto::UploadLockfileRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.uploaded_by_user_id = "user_123";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

                          auto uploaded = service.upload_lockfile(request);

                          Assert::equal(uploaded.failed(), true); }));

  registry.add(TestCase("lockfile service lists lockfiles by project", []
                        {
                          cloud::lockfiles::services::LockfileService service;

                          cloud::lockfiles::dto::UploadLockfileRequest first;
                          first.workspace_id = "workspace_123";
                          first.project_id = "project_123";
                          first.uploaded_by_user_id = "user_123";
                          first.lockfile_json = "{\"packages\":[{\"name\":\"vix/http\"}]}";
                          first.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          first.source = "vix";

                          cloud::lockfiles::dto::UploadLockfileRequest second;
                          second.workspace_id = "workspace_123";
                          second.project_id = "project_123";
                          second.uploaded_by_user_id = "user_123";
                          second.lockfile_json = "{\"packages\":[{\"name\":\"vix/config\"}]}";
                          second.checksum_sha256 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
                          second.source = "vix";

                          cloud::lockfiles::dto::UploadLockfileRequest third;
                          third.workspace_id = "workspace_123";
                          third.project_id = "project_456";
                          third.uploaded_by_user_id = "user_123";
                          third.lockfile_json = "{\"packages\":[{\"name\":\"vix/other\"}]}";
                          third.checksum_sha256 = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
                          third.source = "vix";

                          service.upload_lockfile(first);
                          service.upload_lockfile(second);
                          service.upload_lockfile(third);

                          cloud::lockfiles::dto::ListLockfilesRequest list;
                          list.workspace_id = "workspace_123";
                          list.project_id = "project_123";

                          auto lockfiles = service.list_lockfiles(list);

                          Assert::equal(lockfiles.ok(), true);
                          Assert::equal(
                              static_cast<int>(lockfiles.value().size()),
                              2); }));

  registry.add(TestCase("lockfile service finds lockfile by id", []
                        {
                          cloud::lockfiles::services::LockfileService service;

                          cloud::lockfiles::dto::UploadLockfileRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.uploaded_by_user_id = "user_123";
                          request.lockfile_json = "{\"packages\":[]}";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          request.source = "vix";

                          auto uploaded = service.upload_lockfile(request);

                          cloud::lockfiles::dto::LockfileLookupRequest lookup;
                          lookup.workspace_id = "workspace_123";
                          lookup.project_id = "project_123";
                          lookup.lockfile_id = uploaded.value().id;

                          auto found = service.find_lockfile(lookup);

                          Assert::equal(found.ok(), true);
                          Assert::equal(
                              found.value().id,
                              uploaded.value().id); }));

  return TestRunner::run_all_and_exit();
}
