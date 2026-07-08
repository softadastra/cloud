/**
 *
 *  @file AuthController.hpp
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
#ifndef cloud_auth_controller_hpp
#define cloud_auth_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::auth::controllers
{
  class AuthController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::auth::controllers

#endif // cloud_auth_controller_hpp
