/**
 *
 *  @file MembersController.hpp
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
#ifndef cloud_members_controller_hpp
#define cloud_members_controller_hpp

namespace vix
{
  class App;
}

namespace cloud::members::controllers
{
  class MembersController
  {
  public:
    static void register_routes(vix::App &app);
  };
} // namespace cloud::members::controllers

#endif // cloud_members_controller_hpp
