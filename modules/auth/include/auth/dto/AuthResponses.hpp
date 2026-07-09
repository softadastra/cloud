/**
 *
 *  @file AuthResponses.hpp
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
#ifndef cloud_auth_dto_auth_responses_hpp
#define cloud_auth_dto_auth_responses_hpp

#include <cstdint>
#include <string>

#include <vix.hpp>

namespace cloud::auth::dto
{
  struct AuthUserResponse
  {
    std::string id;
    std::string name;
    std::string display_name;
    std::string email;
    bool email_verified{false};
    bool active{true};
    std::int64_t created_at{0};

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "id", id,
          "name", name,
          "display_name", display_name,
          "email", email,
          "email_verified", email_verified,
          "active", active,
          "created_at", created_at);
    }
  };

  struct LoginResponse
  {
    AuthUserResponse user;
    std::string session_id;
    std::string token;
    std::string token_expires_at;

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "user", user.to_json(),
          "session_id", session_id,
          "token", token,
          "token_expires_at", token_expires_at);
    }
  };

  struct CurrentUserResponse
  {
    AuthUserResponse user;

    vix::json::Json to_json() const
    {
      return vix::json::o(
          "user", user.to_json());
    }
  };
} // namespace cloud::auth::dto

#endif // cloud_auth_dto_auth_responses_hpp
