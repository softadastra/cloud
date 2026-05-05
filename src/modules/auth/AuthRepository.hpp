/**
 *
 *  @file AuthRepository.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_REPOSITORY_HPP
#define SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_REPOSITORY_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <vix/db/db.hpp>

namespace softadastra::cloud::modules::auth
{
  /**
   * @brief User record used by AuthService.
   *
   * This is intentionally small and contains only the fields required for
   * authentication and dashboard identity.
   */
  struct AuthUser
  {
    std::int64_t id{0};
    std::string public_id{};
    std::string email{};
    std::string password_hash{};
    std::string name{};
    std::string role{};
    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return id > 0 &&
             !public_id.empty() &&
             !email.empty() &&
             !password_hash.empty() &&
             !name.empty();
    }
  };

  /**
   * @brief Repository for authentication persistence.
   *
   * AuthRepository owns all SQL access related to dashboard users.
   *
   * It does not contain authentication business rules. It only reads and
   * writes user records.
   */
  class AuthRepository final
  {
  public:
    /**
     * @brief Creates an auth repository.
     *
     * @param database Shared Vix database instance.
     */
    explicit AuthRepository(
        std::shared_ptr<vix::db::Database> database);

    /**
     * @brief Returns true if a user exists for the given email.
     *
     * @param email User email.
     * @return true if the email already exists.
     */
    [[nodiscard]] bool email_exists(
        const std::string &email) const;

    /**
     * @brief Creates a dashboard user.
     *
     * @param public_id Public user id.
     * @param email User email.
     * @param password_hash Password hash.
     * @param name User display name.
     * @param now_ms Creation/update timestamp.
     * @return true on success.
     */
    [[nodiscard]] bool create_user(
        const std::string &public_id,
        const std::string &email,
        const std::string &password_hash,
        const std::string &name,
        std::int64_t now_ms);

    /**
     * @brief Finds a user by email.
     *
     * @param email User email.
     * @return AuthUser if found.
     */
    [[nodiscard]] std::optional<AuthUser> find_by_email(
        const std::string &email) const;

    /**
     * @brief Finds a user by public id.
     *
     * @param public_id Public user id.
     * @return AuthUser if found.
     */
    [[nodiscard]] std::optional<AuthUser> find_by_public_id(
        const std::string &public_id) const;

  private:
    std::shared_ptr<vix::db::Database> database_;

    [[nodiscard]] static AuthUser user_from_current_row(
        const vix::db::ResultRow &row);
  };

} // namespace softadastra::cloud::modules::auth

#endif // SOFTADASTRA_CLOUD_MODULES_AUTH_AUTH_REPOSITORY_HPP
