/**
 *
 *  @file TokenService.hpp
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
#ifndef cloud_tokens_services_token_service_hpp
#define cloud_tokens_services_token_service_hpp

#include <tokens/dto/TokenRequests.hpp>
#include <tokens/dto/TokenResponses.hpp>
#include <tokens/support/TokenErrors.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace cloud::tokens::services
{
  template <typename T>
  class TokenResult
  {
  public:
    static TokenResult success(T value)
    {
      TokenResult result;
      result.ok_ = true;
      result.value_ = std::move(value);
      return result;
    }

    static TokenResult failure(
        support::TokenError error)
    {
      TokenResult result;
      result.ok_ = false;
      result.error_ = std::move(error);
      return result;
    }

    bool ok() const
    {
      return ok_;
    }

    bool failed() const
    {
      return !ok_;
    }

    const T &value() const
    {
      return value_;
    }

    const support::TokenError &error() const
    {
      return error_;
    }

  private:
    bool ok_{false};
    T value_{};
    support::TokenError error_{};
  };

  class TokenService
  {
  public:
    TokenService();
    ~TokenService();

    TokenService(const TokenService &) = delete;
    TokenService &operator=(const TokenService &) = delete;

    TokenService(TokenService &&) noexcept;
    TokenService &operator=(TokenService &&) noexcept;

    TokenResult<dto::TokenResponse> create_token(
        const dto::CreateTokenRequest &request);

    TokenResult<dto::TokenResponse> revoke_token(
        const dto::RevokeTokenRequest &request);

    TokenResult<dto::TokenResponse> find_token(
        const dto::TokenLookupRequest &request) const;

    TokenResult<std::vector<dto::TokenResponse>> list_tokens(
        const dto::ListTokensRequest &request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace cloud::tokens::services

#endif // cloud_tokens_services_token_service_hpp
