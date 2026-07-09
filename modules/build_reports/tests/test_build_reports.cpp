/**
 *
 *  @file test_build_reports.cpp
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
#include <build_reports/BuildReportsModule.hpp>
#include <build_reports/services/BuildReportService.hpp>

#include <string>

#include <vix/tests/tests.hpp>

int main()
{
  using namespace vix::tests;

  auto &registry = TestRegistry::instance();
  registry.clear();

  registry.add(TestCase("build reports module exposes its name", []
                        { Assert::equal(
                              std::string(cloud::build_reports::BuildReportsModule::name()),
                              std::string("build_reports")); }));

  registry.add(TestCase("build report service submits a valid report", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.submitted_by_user_id = "user_123";
                          request.status = "success";
                          request.target = "cloud-api";
                          request.profile = "release";
                          request.branch = "main";
                          request.commit_sha = "abc123";
                          request.toolchain = "gcc-14";
                          request.summary_json = "{\"message\":\"Build completed\"}";
                          request.diagnostics_json = "[]";
                          request.duration_ms = 1450;
                          request.warnings_count = 0;
                          request.errors_count = 0;

                          auto submitted = service.submit_build_report(request);

                          Assert::equal(submitted.ok(), true);
                          Assert::equal(
                              submitted.value().workspace_id,
                              std::string("workspace_123"));
                          Assert::equal(
                              submitted.value().project_id,
                              std::string("project_123"));
                          Assert::equal(
                              submitted.value().submitted_by_user_id,
                              std::string("user_123"));
                          Assert::equal(
                              submitted.value().status,
                              std::string("success"));
                          Assert::equal(
                              submitted.value().target,
                              std::string("cloud-api"));
                          Assert::equal(
                              submitted.value().profile,
                              std::string("release")); }));

  registry.add(TestCase("build report service uses default values", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.submitted_by_user_id = "user_123";
                          request.target = "cloud-api";

                          auto submitted = service.submit_build_report(request);

                          Assert::equal(submitted.ok(), true);
                          Assert::equal(
                              submitted.value().status,
                              std::string("success"));
                          Assert::equal(
                              submitted.value().profile,
                              std::string("dev"));
                          Assert::equal(
                              submitted.value().summary_json,
                              std::string("{}"));
                          Assert::equal(
                              submitted.value().diagnostics_json,
                              std::string("[]")); }));

  registry.add(TestCase("build report service rejects invalid status", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.submitted_by_user_id = "user_123";
                          request.status = "unknown";
                          request.target = "cloud-api";

                          auto submitted = service.submit_build_report(request);

                          Assert::equal(submitted.failed(), true); }));

  registry.add(TestCase("build report service rejects missing target", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.submitted_by_user_id = "user_123";
                          request.status = "success";

                          auto submitted = service.submit_build_report(request);

                          Assert::equal(submitted.failed(), true); }));

  registry.add(TestCase("build report service rejects negative counters", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.submitted_by_user_id = "user_123";
                          request.status = "failed";
                          request.target = "cloud-api";
                          request.duration_ms = -1;

                          auto submitted = service.submit_build_report(request);

                          Assert::equal(submitted.failed(), true); }));

  registry.add(TestCase("build report service lists reports by project", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest first;
                          first.workspace_id = "workspace_123";
                          first.project_id = "project_123";
                          first.submitted_by_user_id = "user_123";
                          first.status = "success";
                          first.target = "cloud-api";

                          cloud::build_reports::dto::SubmitBuildReportRequest second;
                          second.workspace_id = "workspace_123";
                          second.project_id = "project_123";
                          second.submitted_by_user_id = "user_123";
                          second.status = "warning";
                          second.target = "cloud-worker";
                          second.warnings_count = 2;

                          cloud::build_reports::dto::SubmitBuildReportRequest third;
                          third.workspace_id = "workspace_123";
                          third.project_id = "project_456";
                          third.submitted_by_user_id = "user_123";
                          third.status = "failed";
                          third.target = "other-api";
                          third.errors_count = 1;

                          service.submit_build_report(first);
                          service.submit_build_report(second);
                          service.submit_build_report(third);

                          cloud::build_reports::dto::ListBuildReportsRequest list;
                          list.workspace_id = "workspace_123";
                          list.project_id = "project_123";

                          auto reports = service.list_build_reports(list);

                          Assert::equal(reports.ok(), true);
                          Assert::equal(
                              static_cast<int>(reports.value().size()),
                              2); }));

  registry.add(TestCase("build report service finds report by id", []
                        {
                          cloud::build_reports::services::BuildReportService service;

                          cloud::build_reports::dto::SubmitBuildReportRequest request;
                          request.workspace_id = "workspace_123";
                          request.project_id = "project_123";
                          request.submitted_by_user_id = "user_123";
                          request.status = "success";
                          request.target = "cloud-api";

                          auto submitted = service.submit_build_report(request);

                          cloud::build_reports::dto::BuildReportLookupRequest lookup;
                          lookup.workspace_id = "workspace_123";
                          lookup.project_id = "project_123";
                          lookup.build_report_id = submitted.value().id;

                          auto found = service.find_build_report(lookup);

                          Assert::equal(found.ok(), true);
                          Assert::equal(
                              found.value().id,
                              submitted.value().id); }));

  return TestRunner::run_all_and_exit();
}
