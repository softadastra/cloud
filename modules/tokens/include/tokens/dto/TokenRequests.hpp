/**
 *
 *  @file TokenRequests.hpp
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
#ifndef cloud_tokens_dto_token_requests_hpp
#define cloud_tokens_dto_token_requests_hpp

#include <string>
#include <vector>

namespace cloud::tokens::dto
{
  struct CreateTokenRequest
  {
    std::string workspace_id;
    std::string user_id;
    std::string name;
    std::vector<std::string> scopes;
  };

  struct RevokeTokenRequest
  {
    std::string token_id;
    std::string workspace_id;
    std::string user_id;
  };

  struct ListTokensRequest
  {
    std::string workspace_id;
    std::string user_id;
  };

  struct TokenLookupRequest
  {
    std::string token_id;
    std::string workspace_id;
    std::string user_id;
  };
} // namespace cloud::tokens::dto

#endif // cloud_tokens_dto_token_requests_hpp
