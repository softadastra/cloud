/*
 * AuthRepository.cpp
 */

#include "modules/auth/AuthRepository.hpp"

#include <stdexcept>

#include <vix/log/Log.hpp>

namespace softadastra::cloud::modules::auth
{
  AuthRepository::AuthRepository(
      std::shared_ptr<vix::db::Database> database)
      : database_(std::move(database))
  {
    if (database_ == nullptr)
    {
      throw std::invalid_argument(
          "AuthRepository requires a valid database instance");
    }
  }

  bool AuthRepository::email_exists(
      const std::string &email) const
  {
    auto rows =
        database_->query(
            "SELECT id FROM users WHERE email = ? LIMIT 1",
            email);

    return rows->next();
  }

  bool AuthRepository::create_user(
      const std::string &public_id,
      const std::string &email,
      const std::string &password_hash,
      const std::string &name,
      std::int64_t now_ms)
  {
    try
    {
      database_->exec(
          "INSERT INTO users ("
          "public_id, email, password_hash, name, role, created_at, updated_at"
          ") VALUES (?, ?, ?, ?, ?, ?, ?)",
          public_id,
          email,
          password_hash,
          name,
          std::string{"user"},
          now_ms,
          now_ms);

      return true;
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to create auth user",
          "email", email,
          "error", e.what());

      return false;
    }
  }

  std::optional<AuthUser> AuthRepository::find_by_email(
      const std::string &email) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "id, public_id, email, password_hash, name, role, created_at, updated_at "
            "FROM users "
            "WHERE email = ? "
            "LIMIT 1",
            email);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return user_from_current_row(rows->row());
  }

  std::optional<AuthUser> AuthRepository::find_by_public_id(
      const std::string &public_id) const
  {
    auto rows =
        database_->query(
            "SELECT "
            "id, public_id, email, password_hash, name, role, created_at, updated_at "
            "FROM users "
            "WHERE public_id = ? "
            "LIMIT 1",
            public_id);

    if (!rows->next())
    {
      return std::nullopt;
    }

    return user_from_current_row(rows->row());
  }

  AuthUser AuthRepository::user_from_current_row(
      const vix::db::Row &row)
  {
    AuthUser user;

    user.id = row.getInt64(0);
    user.public_id = row.getString(1);
    user.email = row.getString(2);
    user.password_hash = row.getString(3);
    user.name = row.getString(4);
    user.role = row.getString(5);
    user.created_at = row.getInt64(6);
    user.updated_at = row.getInt64(7);

    return user;
  }

} // namespace softadastra::cloud::modules::auth
