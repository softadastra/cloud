/*
 * AuthService.cpp
 */

#include "modules/auth/AuthService.hpp"

#include <functional>
#include <sstream>
#include <string>

#include <vix/log/Log.hpp>

#include "security/TokenService.hpp"
#include "utils/IdGenerator.hpp"
#include "utils/Time.hpp"

namespace softadastra::cloud::modules::auth
{
  namespace
  {
    constexpr std::size_t MIN_PASSWORD_LENGTH = 8;

    [[nodiscard]] std::string make_password_hash_payload(
        const std::string &password)
    {
      /*
       * MVP note:
       * This is intentionally isolated behind AuthService::hash_password().
       * Later we can replace the implementation with a stronger password
       * hashing strategy without changing controllers or repositories.
       */
      const std::hash<std::string> hasher;
      const auto value = hasher("softadastra-cloud:" + password);

      std::ostringstream out;
      out << "v1$" << value;

      return out.str();
    }
  }

  AuthService::AuthService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        repository_(state.database)
  {
  }

  AuthResult AuthService::register_user(
      const std::string &name,
      const std::string &email,
      const std::string &password)
  {
    if (name.empty())
    {
      return validation_error("Name is required");
    }

    if (!is_valid_email(email))
    {
      return validation_error("A valid email is required");
    }

    if (!is_valid_password(password))
    {
      return validation_error("Password must contain at least 8 characters");
    }

    if (repository_.email_exists(email))
    {
      return conflict("Email is already registered");
    }

    const std::string public_id =
        softadastra::cloud::utils::IdGenerator::generate("usr");

    const std::string password_hash =
        hash_password(password);

    const auto now =
        softadastra::cloud::utils::Time::now_ms();

    const bool created =
        repository_.create_user(
            public_id,
            email,
            password_hash,
            name,
            now);

    if (!created)
    {
      return internal_error("Failed to create user");
    }

    AuthResult result;
    result.success = true;
    result.status = 201;
    result.error.clear();
    result.message = "User created";
    result.user_public_id = public_id;
    result.email = email;
    result.name = name;

    return result;
  }

  AuthResult AuthService::login(
      const std::string &email,
      const std::string &password)
  {
    if (!is_valid_email(email))
    {
      return validation_error("A valid email is required");
    }

    if (password.empty())
    {
      return validation_error("Password is required");
    }

    const auto user =
        repository_.find_by_email(email);

    if (!user.has_value())
    {
      return unauthorized("Invalid email or password");
    }

    if (!verify_password(password, user->password_hash))
    {
      return unauthorized("Invalid email or password");
    }

    std::string token;

    try
    {
      token =
          softadastra::cloud::security::TokenService::issue_user_token(
              state_.config,
              user->public_id,
              user->email);
    }
    catch (const std::exception &e)
    {
      vix::log::logf(
          vix::log::LogLevel::Error,
          "failed to issue auth token",
          "error", e.what());

      return internal_error("Failed to issue authentication token");
    }

    AuthResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Authenticated";
    result.user_public_id = user->public_id;
    result.email = user->email;
    result.name = user->name;
    result.token = std::move(token);

    return result;
  }

  bool AuthService::is_valid_email(
      const std::string &email) noexcept
  {
    const auto at = email.find('@');
    const auto dot = email.rfind('.');

    return !email.empty() &&
           at != std::string::npos &&
           dot != std::string::npos &&
           at > 0 &&
           dot > at + 1 &&
           dot + 1 < email.size();
  }

  bool AuthService::is_valid_password(
      const std::string &password) noexcept
  {
    return password.size() >= MIN_PASSWORD_LENGTH;
  }

  std::string AuthService::hash_password(
      const std::string &password)
  {
    return make_password_hash_payload(password);
  }

  bool AuthService::verify_password(
      const std::string &password,
      const std::string &password_hash)
  {
    return hash_password(password) == password_hash;
  }

  AuthResult AuthService::validation_error(
      std::string message)
  {
    AuthResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  AuthResult AuthService::unauthorized(
      std::string message)
  {
    AuthResult result;
    result.success = false;
    result.status = 401;
    result.error = "invalid_credentials";
    result.message = std::move(message);
    return result;
  }

  AuthResult AuthService::conflict(
      std::string message)
  {
    AuthResult result;
    result.success = false;
    result.status = 409;
    result.error = "conflict";
    result.message = std::move(message);
    return result;
  }

  AuthResult AuthService::internal_error(
      std::string message)
  {
    AuthResult result;
    result.success = false;
    result.status = 500;
    result.error = "internal_error";
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::auth
