/**
 *
 *  @file AuthErrors.hpp
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
#ifndef cloud_auth_support_auth_errors_hpp
#define cloud_auth_support_auth_errors_hpp

#include <string>

#include <rix.hpp>
#include <vix.hpp>

namespace cloud::auth::support
{
  int http_status_for_auth_error(
      const rixlib::auth::AuthError &error);

  std::string public_code_for_auth_error(
      const rixlib::auth::AuthError &error);

  std::string public_message_for_auth_error(
      const rixlib::auth::AuthError &error);

  void write_auth_error(
      vix::Response &res,
      const rixlib::auth::AuthError &error);
} // namespace cloud::auth::support

#endif // cloud_auth_support_auth_errors_hpp
