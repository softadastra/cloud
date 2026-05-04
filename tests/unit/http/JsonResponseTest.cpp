/**
 *
 *  @file JsonResponseTest.cpp
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

#include <vix.hpp>
#include <vix/http/Response.hpp>
#include <vix/http/Status.hpp>
#include <vix/tests/tests.hpp>

#include "http/JsonResponse.hpp"

namespace
{
  struct TestResponse
  {
    vix::http::Response native{};
    vix::Response wrapped;

    TestResponse()
        : native(),
          wrapped(native)
    {
    }
  };

  void register_json_response_tests()
  {
    vix::tests::TestSuite suite{"http.JsonResponse"};

    suite.add(
        vix::tests::TestCase{
            "ok writes 200 response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::ok(
                  response.wrapped,
                  "ok");

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "ok response should use status 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "ok response should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "created writes 201 response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::created(
                  response.wrapped,
                  vix::json::obj({
                      "id",
                      "test",
                  }));

              vix::tests::Assert::equal(
                  vix::http::CREATED,
                  response.native.status(),
                  "created response should use status 201");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "created response should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "validation_error writes 400 response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::validation_error(
                  response.wrapped,
                  "invalid input");

              vix::tests::Assert::equal(
                  vix::http::BAD_REQUEST,
                  response.native.status(),
                  "validation_error should use status 400");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "validation_error should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "unauthorized writes 401 response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::unauthorized(
                  response.wrapped,
                  "missing token");

              vix::tests::Assert::equal(
                  vix::http::UNAUTHORIZED,
                  response.native.status(),
                  "unauthorized should use status 401");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "unauthorized should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "internal_error writes 500 response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::internal_error(
                  response.wrapped,
                  "server error");

              vix::tests::Assert::equal(
                  vix::http::INTERNAL_ERROR,
                  response.native.status(),
                  "internal_error should use status 500");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "internal_error should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "error writes custom status response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::error(
                  response.wrapped,
                  vix::http::NOT_FOUND,
                  "not_found",
                  "Resource not found");

              vix::tests::Assert::equal(
                  vix::http::NOT_FOUND,
                  response.native.status(),
                  "custom error should use given status");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "custom error should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "data writes 200 response",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::data(
                  response.wrapped,
                  "loaded",
                  vix::json::obj({
                      "value",
                      42,
                  }));

              vix::tests::Assert::equal(
                  vix::http::OK,
                  response.native.status(),
                  "data response should use status 200");

              vix::tests::Assert::is_false(
                  response.native.body().empty(),
                  "data response should write a body");
            }});

    suite.add(
        vix::tests::TestCase{
            "json responses set content type",
            []
            {
              TestResponse response;

              softadastra::cloud::http::JsonResponse::ok(
                  response.wrapped,
                  "ok");

              vix::tests::Assert::equal(
                  std::string{"application/json; charset=utf-8"},
                  response.native.header("Content-Type"),
                  "json response should set application/json content type");
            }});

    vix::tests::TestRegistry::instance().add(std::move(suite));
  }
}

int main()
{
  register_json_response_tests();

  return vix::tests::TestRunner::run_all_and_exit();
}
