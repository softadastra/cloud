/**
 *
 *  @file AuthRequests.hpp
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
#ifndef cloud_auth_dto_auth_requests_hpp
#define cloud_auth_dto_auth_requests_hpp

#include <string>

namespace cloud::auth::dto
{
  struct RegisterRequest
  {
    std::string name;
    std::string email;
    std::string password;
  };

  struct LoginRequest
  {
    std::string email;
    std::string password;
  };

  struct LogoutRequest
  {
    std::string session_id;
  };

  struct SessionRequest
  {
    std::string session_id;
  };
} // namespace cloud::auth::dto

#endif // cloud_auth_dto_auth_requests_hpp
