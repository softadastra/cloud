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
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <vix/config/Config.hpp>
#include <vix/crypto/hex.hpp>
#include <vix/crypto/random.hpp>
#include <vix/db/Sha256.hpp>
#include <vix/db/db.hpp>
#include <vix/json.hpp>

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

    std::string make_development_token_value()
    {
      static std::atomic<std::uint64_t> counter{0};

      std::ostringstream stream;
      stream << "sdc_"
             << now_timestamp()
             << "_"
             << counter.fetch_add(1);

      return stream.str();
    }

    std::string make_secure_token_value()
    {
      std::array<std::uint8_t, 32> bytes{};
      auto random = vix::crypto::random_bytes(bytes);

      if (!random.ok())
      {
        return make_development_token_value();
      }

      return "sdc_" + vix::crypto::hex_lower(bytes);
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

    std::string scopes_to_json(
        const std::vector<std::string> &scopes)
    {
      auto json = vix::json::a();

      for (const auto &scope : scopes)
      {
        json.push_back(scope);
      }

      return json.dump();
    }

    std::vector<std::string> scopes_from_json(
        const std::string &value)
    {
      std::vector<std::string> scopes;

      if (value.empty())
      {
        return scopes;
      }

      auto parsed = vix::json::Json::parse(value);

      if (!parsed.is_array())
      {
        return scopes;
      }

      for (const auto &scope : parsed)
      {
        if (scope.is_string())
        {
          scopes.push_back(scope.get<std::string>());
        }
      }

      return scopes;
    }
  } // namespace

  class TokenService::Impl
  {
  public:
    Impl()
    {
      const char *use_db = std::getenv("SOFTADASTRA_CLOUD_USE_DB");

      if (use_db != nullptr && std::string(use_db) == "1")
      {
        vix::config::Config cfg{".env"};
        db = std::make_unique<vix::db::Database>(cfg);
      }
    }

    bool persistent() const
    {
      return db != nullptr;
    }

    bool workspace_exists(
        const std::string &workspace_id) const
    {
      auto rows = db->query(
          "SELECT id FROM workspaces WHERE id = ? LIMIT 1",
          workspace_id);

      return rows->next();
    }

    dto::TokenResponse row_to_token(
        const vix::db::ResultRow &row,
        bool include_value = false,
        const std::string &value = "") const
    {
      dto::TokenResponse token;
      token.id = row.getString(0);
      token.workspace_id = row.getString(1);
      token.user_id = row.getString(2);
      token.name = row.getString(3);
      token.value = include_value ? value : "";
      token.scopes = scopes_from_json(row.getString(4));
      token.revoked = row.getInt64(5) != 0;
      token.created_at = row.getInt64(6);
      token.updated_at = row.getInt64(7);
      token.expires_at = row.getInt64(8);
      return token;
    }

    std::unique_ptr<vix::db::Database> db;
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

    if (impl_->persistent())
    {
      try
      {
        if (!impl_->workspace_exists(request.workspace_id))
        {
          return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::MissingWorkspace,
                                                           "Workspace not found."});
        }

        const auto timestamp = now_timestamp();
        const auto value = make_secure_token_value();
        const auto token_hash = vix::db::sha256_hex(value);

        dto::TokenResponse token;
        token.id = make_token_id();
        token.workspace_id = request.workspace_id;
        token.user_id = request.user_id;
        token.name = request.name;
        token.value = value;
        token.scopes = request.scopes;
        token.revoked = false;
        token.created_at = timestamp;
        token.updated_at = timestamp;
        token.expires_at = timestamp + (60 * 60 * 24 * 90);

        impl_->db->exec(
            "INSERT INTO access_tokens "
            "(id, workspace_id, user_id, name, token_hash, scopes_json, expires_at, created_at, updated_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
            token.id,
            token.workspace_id,
            token.user_id,
            token.name,
            token_hash,
            scopes_to_json(token.scopes),
            token.expires_at,
            token.created_at,
            token.updated_at);

        return TokenResult<dto::TokenResponse>::success(token);
      }
      catch (const std::exception &error)
      {
        (void)error;
        return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::InvalidScopes,
                                                         "Could not create access token."});
      }
    }

    const auto timestamp = now_timestamp();

    dto::TokenResponse token;
    token.id = make_token_id();
    token.workspace_id = request.workspace_id;
    token.user_id = request.user_id;
    token.name = request.name;
    token.value = make_development_token_value();
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

    if (impl_->persistent())
    {
      auto current = find_token({request.token_id, request.workspace_id, request.user_id});

      if (current.failed())
      {
        return current;
      }

      if (current.value().revoked)
      {
        return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::TokenAlreadyRevoked,
                                                         "Token is already revoked."});
      }

      auto token = current.value();
      token.revoked = true;
      token.updated_at = now_timestamp();

      impl_->db->exec(
          "UPDATE access_tokens SET revoked_at = ?, updated_at = ? WHERE id = ? AND workspace_id = ? AND user_id = ?",
          token.updated_at,
          token.updated_at,
          token.id,
          token.workspace_id,
          token.user_id);

      return TokenResult<dto::TokenResponse>::success(token);
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

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, user_id, name, scopes_json, CASE WHEN revoked_at IS NULL THEN 0 ELSE 1 END, created_at, updated_at, expires_at "
          "FROM access_tokens WHERE id = ? AND workspace_id = ? AND user_id = ? LIMIT 1",
          request.token_id,
          request.workspace_id,
          request.user_id);

      if (!rows->next())
      {
        return TokenResult<dto::TokenResponse>::failure({support::TokenErrorCode::TokenNotFound,
                                                         "Token not found."});
      }

      return TokenResult<dto::TokenResponse>::success(
          impl_->row_to_token(rows->row()));
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

    if (impl_->persistent())
    {
      auto rows = impl_->db->query(
          "SELECT id, workspace_id, user_id, name, scopes_json, CASE WHEN revoked_at IS NULL THEN 0 ELSE 1 END, created_at, updated_at, expires_at "
          "FROM access_tokens WHERE workspace_id = ? AND user_id = ? ORDER BY created_at",
          request.workspace_id,
          request.user_id);

      while (rows->next())
      {
        tokens.push_back(impl_->row_to_token(rows->row()));
      }

      return TokenResult<std::vector<dto::TokenResponse>>::success(tokens);
    }

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
