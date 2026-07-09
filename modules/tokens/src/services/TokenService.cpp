/**
 *
 *  @file TokenService.cpp
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
#include <tokens/services/TokenService.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>

namespace cloud::tokens::services
{
  namespace
  {
    std::int64_t now_timestamp()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    std::string make_token_id()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "token_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    std::string make_token_value()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "sdc_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    bool valid_name(
        const std::string &name)
    {
      return name.size() >= 2 && name.size() <= 80;
    }

    bool valid_scope(
        const std::string &scope)
    {
      return scope == "registry:read" ||
             scope == "registry:write" ||
             scope == "projects:read" ||
             scope == "projects:write" ||
             scope == "lockfiles:write" ||
             scope == "build_reports:write";
    }

    bool valid_scopes(
        const std::vector<std::string> &scopes)
    {
      if (scopes.empty())
      {
        return false;
      }

      for (const auto &scope : scopes)
      {
        if (!valid_scope(scope))
        {
          return false;
        }
      }

      return true;
    }
  } // namespace

  class TokenService::Impl
  {
  public:
    std::unordered_map<std::string, dto::TokenResponse> tokens_by_id;
  };

  TokenService::TokenService()
      : impl_(std::make_unique<Impl>())
  {
  }

  TokenService::~TokenService() = default;

  TokenService::TokenService(TokenService &&) noexcept = default;

  TokenService &TokenService::operator=(TokenService &&) noexcept = default;

  TokenResult<dto::TokenResponse> TokenService::create_token(
      const dto::CreateTokenRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingWorkspace,
                                                       "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingUser,
                                                       "User is required."});
    }

    if (!valid_name(request.name))
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingName,
                                                       "Token name must contain between 2 and 80 characters."});
    }

    if (!valid_scopes(request.scopes))
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::InvalidScopes,
                                                       "Token scopes are invalid."});
    }

    const auto timestamp = now_timestamp();

    dto::TokenResponse token;
    token.id = make_token_id();
    token.workspace_id = request.workspace_id;
    token.user_id = request.user_id;
    token.name = request.name;
    token.value = make_token_value();
    token.scopes = request.scopes;
    token.revoked = false;
    token.created_at = timestamp;
    token.updated_at = timestamp;
    token.expires_at = timestamp + (60 * 60 * 24 * 90);

    impl_->tokens_by_id[token.id] = token;

    return TokenResult<dto::TokenResponse>::success(token);
  }

  TokenResult<dto::TokenResponse> TokenService::revoke_token(
      const dto::RevokeTokenRequest &request)
  {
    if (request.workspace_id.empty())
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingWorkspace,
                                                       "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingUser,
                                                       "User is required."});
    }

    auto item = impl_->tokens_by_id.find(request.token_id);

    if (item == impl_->tokens_by_id.end() ||
        item->second.workspace_id != request.workspace_id ||
        item->second.user_id != request.user_id)
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::TokenNotFound,
                                                       "Token not found."});
    }

    if (item->second.revoked)
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::TokenAlreadyRevoked,
                                                       "Token is already revoked."});
    }

    item->second.revoked = true;
    item->second.updated_at = now_timestamp();

    return TokenResult<dto::TokenResponse>::success(item->second);
  }

  TokenResult<dto::TokenResponse> TokenService::find_token(
      const dto::TokenLookupRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingWorkspace,
                                                       "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingUser,
                                                       "User is required."});
    }

    auto item = impl_->tokens_by_id.find(request.token_id);

    if (item == impl_->tokens_by_id.end() ||
        item->second.workspace_id != request.workspace_id ||
        item->second.user_id != request.user_id)
    {
      return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::TokenNotFound,
                                                       "Token not found."});
    }

    return TokenResult<dto::TokenResponse>::success(item->second);
  }

  TokenResult<std::vector<dto::TokenResponse>> TokenService::list_tokens(
      const dto::ListTokensRequest &request) const
  {
    if (request.workspace_id.empty())
    {
      return TokenResult<std::vector<dto::TokenResponse>>::failure({support::TokenErrorCode::MissingWorkspace,
                                                                    "Workspace is required."});
    }

    if (request.user_id.empty())
    {
      return TokenResult<std::vector<dto::TokenResponse>>::failure({support::TokenErrorCode::MissingUser,
                                                                    "User is required."});
    }

    std::vector<dto::TokenResponse> tokens;

    for (const auto &entry : impl_->tokens_by_id)
    {
      if (entry.second.workspace_id == request.workspace_id &&
          entry.second.user_id == request.user_id)
      {
        tokens.push_back(entry.second);
      }
    }

    std::sort(
        tokens.begin(),
        tokens.end(),
        [](const auto &left, const auto &right)
        {
          return left.created_at < right.created_at;
        });

    return TokenResult<std::vector<dto::TokenResponse>>::success(tokens);
  }
} // namespace cloud::tokens::services
