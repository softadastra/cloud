/**
 *
 *  @file test_package_versions.cpp
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
#include <package_versions/PackageVersionsModule.hpp>
#include <package_versions/services/PackageVersionService.hpp>
#include <package_versions/support/PackageVersionErrors.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("package_versions module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::package_versions::PackageVersionsModule::name()),
                              std::string("package_versions")); }));

  registry.add(TestCase("package version service publishes a valid version", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest request;
                          request.workspace_id = "workspace_123";
                          request.package_id = "package_123";
                          request.published_by_user_id = "user_123";
                          request.version = "1.0.0";
                          request.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          request.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          request.size_bytes = 2048;

                          auto published = service.publish_package_version(request);

                          Assert::equal(published.ok(), true);
                          Assert::equal(
                              published.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              published.value().package_id,
                              std::string("package_123"));
                          Assert::equal(
                              published.value().version,
                              std::string("1.0.0"));
                          Assert::equal(
                              published.value().status,
                              std::string("published")); }));

  registry.add(TestCase("package version service rejects invalid checksum", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest request;
                          request.workspace_id = "workspace_123";
                          request.package_id = "package_123";
                          request.published_by_user_id = "user_123";
                          request.version = "1.0.0";
                          request.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          request.checksum_sha256 = "invalid";
                          request.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          request.size_bytes = 2048;

                          auto published = service.publish_package_version(request);

                          Assert::equal(published.failed(), true); }));

  registry.add(TestCase("package version service rejects duplicate version", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest request;
                          request.workspace_id = "workspace_123";
                          request.package_id = "package_123";
                          request.published_by_user_id = "user_123";
                          request.version = "1.0.0";
                          request.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          request.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          request.size_bytes = 2048;

                          auto first = service.publish_package_version(request);
                          auto second = service.publish_package_version(request);

                          Assert::equal(first.ok(), true);
                          Assert::equal(second.failed(), true);
                          Assert::equal(
                              cloud::package_versions::support::http_status_for_package_version_error(second.error()),
                              409);
                          Assert::equal(
                              cloud::package_versions::support::public_code_for_package_version_error(second.error()),
                              std::string("package_version_already_exists")); }));

  registry.add(TestCase("package version service allows same version for different packages", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest first;
                          first.workspace_id = "workspace_123";
                          first.package_id = "package_123";
                          first.published_by_user_id = "user_123";
                          first.version = "1.0.0";
                          first.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          first.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          first.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          first.size_bytes = 2048;

                          cloud::package_versions::dto::PublishPackageVersionRequest second;
                          second.workspace_id = "workspace_123";
                          second.package_id = "package_456";
                          second.published_by_user_id = "user_123";
                          second.version = "1.0.0";
                          second.archive_url = "storage/packages/package_456/1.0.0/archive.tar.gz";
                          second.checksum_sha256 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
                          second.manifest_json = "{\"name\":\"vix/config\",\"version\":\"1.0.0\"}";
                          second.size_bytes = 4096;

                          auto first_published = service.publish_package_version(first);
                          auto second_published = service.publish_package_version(second);

                          Assert::equal(first_published.ok(), true);
                          Assert::equal(second_published.ok(), true); }));

  registry.add(TestCase("package version service lists versions by package", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest first;
                          first.workspace_id = "workspace_123";
                          first.package_id = "package_123";
                          first.published_by_user_id = "user_123";
                          first.version = "1.0.0";
                          first.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          first.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          first.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          first.size_bytes = 2048;

                          cloud::package_versions::dto::PublishPackageVersionRequest second;
                          second.workspace_id = "workspace_123";
                          second.package_id = "package_123";
                          second.published_by_user_id = "user_123";
                          second.version = "1.1.0";
                          second.archive_url = "storage/packages/package_123/1.1.0/archive.tar.gz";
                          second.checksum_sha256 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
                          second.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.1.0\"}";
                          second.size_bytes = 4096;

                          cloud::package_versions::dto::PublishPackageVersionRequest third;
                          third.workspace_id = "workspace_123";
                          third.package_id = "package_456";
                          third.published_by_user_id = "user_123";
                          third.version = "1.0.0";
                          third.archive_url = "storage/packages/package_456/1.0.0/archive.tar.gz";
                          third.checksum_sha256 = "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
                          third.manifest_json = "{\"name\":\"vix/config\",\"version\":\"1.0.0\"}";
                          third.size_bytes = 1024;

                          service.publish_package_version(first);
                          service.publish_package_version(second);
                          service.publish_package_version(third);

                          cloud::package_versions::dto::ListPackageVersionsRequest list;
                          list.workspace_id = "workspace_123";
                          list.package_id = "package_123";

                          auto versions = service.list_package_versions(list);

                          Assert::equal(versions.ok(), true);
                          Assert::equal(
                              static_cast<int>(versions.value().size()),
                              2); }));

  registry.add(TestCase("package version service finds version by id", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest request;
                          request.workspace_id = "workspace_123";
                          request.package_id = "package_123";
                          request.published_by_user_id = "user_123";
                          request.version = "1.0.0";
                          request.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          request.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          request.size_bytes = 2048;

                          auto published = service.publish_package_version(request);

                          cloud::package_versions::dto::PackageVersionLookupRequest lookup;
                          lookup.workspace_id = "workspace_123";
                          lookup.package_id = "package_123";
                          lookup.version_id = published.value().id;

                          auto found = service.find_package_version(lookup);

                          Assert::equal(found.ok(), true);
                          Assert::equal(
                              found.value().id,
                              published.value().id); }));

  registry.add(TestCase("package version service returns not found for missing version", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PackageVersionLookupRequest lookup;
                          lookup.workspace_id = "workspace_123";
                          lookup.package_id = "package_123";
                          lookup.version_id = "version_missing";

                          auto found = service.find_package_version(lookup);

                          Assert::equal(found.failed(), true);
                          Assert::equal(
                              cloud::package_versions::support::http_status_for_package_version_error(found.error()),
                              404);
                          Assert::equal(
                              cloud::package_versions::support::public_code_for_package_version_error(found.error()),
                              std::string("package_version_not_found")); }));

  registry.add(TestCase("package version service resolves version by number", []
                        {
                          cloud::package_versions::services::PackageVersionService service;

                          cloud::package_versions::dto::PublishPackageVersionRequest request;
                          request.workspace_id = "workspace_123";
                          request.package_id = "package_123";
                          request.published_by_user_id = "user_123";
                          request.version = "1.0.0";
                          request.archive_url = "storage/packages/package_123/1.0.0/archive.tar.gz";
                          request.checksum_sha256 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
                          request.manifest_json = "{\"name\":\"vix/http\",\"version\":\"1.0.0\"}";
                          request.size_bytes = 2048;

                          auto published = service.publish_package_version(request);

                          cloud::package_versions::dto::PackageVersionByNumberRequest resolve;
                          resolve.workspace_id = "workspace_123";
                          resolve.package_id = "package_123";
                          resolve.version = "1.0.0";

                          auto resolved = service.find_package_version_by_number(resolve);

                          Assert::equal(resolved.ok(), true);
                          Assert::equal(
                              resolved.value().id,
                              published.value().id); }));

  return TestRunner::run_all_and_exit();
}
