/**
 *
 *  @file TokenResponses.hpp
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
#ifndef cloud_tokens_dto_token_responses_hpp
#define cloud_tokens_dto_token_responses_hpp

#include <cstdint>
#include <string>
#include <vector>

#include <vix/json.hpp>

namespace cloud::tokens::dto
{
  struct TokenResponse
  {
    std::string id;
    std::string workspace_id;
    std::string user_id;
    std::string name;
    std::string value;
    std::vector<std::string> scopes;
    bool revoked{false};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};
    std::int64_t expires_at{0};

    vix::json::Json to_json() const
    {
      auto scopes_json = vix::json::a();

      for (const auto &scope : scopes)
      {
        scopes_json.push_back(scope);
      }

      return vix::json::o(
          "id", id,
          "workspace_id", workspace_id,
          "user_id", user_id,
          "name", name,
          "value", value,
          "scopes", scopes_json,
          "revoked", revoked,
          "created_at", created_at,
          "updated_at", updated_at,
          "expires_at", expires_at);
    }
  };
} // namespace cloud::tokens::dto

#endif // cloud_tokens_dto_token_responses_hpp
