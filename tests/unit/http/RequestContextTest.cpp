/**
 *
 *  @file RequestContextTest.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra/cloud
 *
 *  Licensed under the Apache License, Version 2.0.
 *
 *  Softadastra Cloud
 *
 */

#include <string>

#include <vix/tests/tests.hpp>

#include "http/RequestContext.hpp"

namespace
{
  void register_request_context_tests()
  {
    vix::tests::TestSuite suite{"http.RequestContext"};

    suite.add(
        vix::tests::TestCase{
            "default context has no request id",
            []
            {
              softadastra::cloud::http::RequestContext context;

              vix::tests::Assert::is_false(
                  context.has_request_id(),
                  "new context should not have a request id");
            }});

    suite.add(
        vix::tests::TestCase{
            "default context is not authenticated",
            []
            {
              softadastra::cloud::http::RequestContext context;

              vix::tests::Assert::equal(
                  std::string{"none"},
                  context.auth_source,
                  "new context should use auth_source=none");

              vix::tests::Assert::is_false(
                  context.has_agent(),
                  "new context should not have an agent");

              vix::tests::Assert::is_false(
                  context.has_project(),
                  "new context should not have a project");
            }});

    suite.add(
        vix::tests::TestCase{
            "request id can be assigned",
            []
            {
              softadastra::cloud::http::RequestContext context;
              context.request_id = "req_test_123";

              vix::tests::Assert::is_true(
                  context.has_request_id(),
                  "context should have a request id");

              vix::tests::Assert::equal(
                  std::string{"req_test_123"},
                  context.request_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "set_agent_auth stores agent and project ids",
            []
            {
              softadastra::cloud::http::RequestContext context;

              context.set_agent_auth(
                  "agt_test",
                  "prj_test");

              vix::tests::Assert::equal(
                  std::string{"agent_api_key"},
                  context.auth_source,
                  "agent auth should set auth_source=agent_api_key");

              vix::tests::Assert::equal(
                  std::string{"agt_test"},
                  context.agent_public_id);

              vix::tests::Assert::equal(
                  std::string{"prj_test"},
                  context.project_public_id);
            }});

    suite.add(
        vix::tests::TestCase{
            "has_agent returns true after set_agent_auth",
            []
            {
              softadastra::cloud::http::RequestContext context;

              context.set_agent_auth(
                  "agt_test",
                  "prj_test");

              vix::tests::Assert::is_true(
                  context.has_agent(),
                  "context should have an agent after set_agent_auth");
            }});

    suite.add(
        vix::tests::TestCase{
            "has_project returns true after set_agent_auth",
            []
            {
              softadastra::cloud::http::RequestContext context;

              context.set_agent_auth(
                  "agt_test",
                  "prj_test");

              vix::tests::Assert::is_true(
                  context.has_project(),
                  "context should have a project after set_agent_auth");
            }});

    suite.add(
        vix::tests::TestCase{
            "clear_auth resets auth state",
            []
            {
              softadastra::cloud::http::RequestContext context;

              context.set_agent_auth(
                  "agt_test",
                  "prj_test");

              context.clear_auth();

              vix::tests::Assert::equal(
                  std::string{"none"},
                  context.auth_source,
                  "clear_auth should reset auth_source");

              vix::tests::Assert::is_true(
                  context.agent_public_id.empty(),
                  "clear_auth should clear agent id");

              vix::tests::Assert::is_true(
                  context.project_public_id.empty(),
                  "clear_auth should clear project id");

              vix::tests::Assert::is_false(
                  context.has_agent(),
                  "clear_auth should make has_agent false");

              vix::tests::Assert::is_false(
                  context.has_project(),
                  "clear_auth should make has_project false");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_request_context_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
