# Vix backend application manifest
# This file describes one executable backend target.
# Vix converts it to an internal CMake project under .vix/generated/app/.

name = "cloud"
type = "executable"
standard = "c++20"
output_dir = "bin"

sources = [
  "src/main.cpp",
  "src/cloud/app/AppBootstrap.cpp",
  "src/cloud/support/HttpResponses.cpp",
  "src/cloud/presentation/routes/RouteRegistry.cpp",
  "src/cloud/presentation/middleware/MiddlewareRegistry.cpp",
  "src/cloud/presentation/controllers/HomeController.cpp",
  "src/cloud/presentation/controllers/HealthController.cpp",
]

include_dirs = [
  "include",
  "src",
]

defines = [
  "VIX_BACKEND_APP=1",
  "VIX_APP_NAME=cloud",
]

compile_options = [
  "$<$<CXX_COMPILER_ID:MSVC>:/W4>",
  "$<$<CXX_COMPILER_ID:MSVC>:/permissive->",
  "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>",
  "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra>",
  "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wpedantic>",
]

link_options = [
]

compile_features = [
  "cxx_std_20",
]

packages = [
  "vix",
]

links = [
  "vix::vix",
  "vix::db",
]

resources = [
  ".env=.env",
  "storage=storage",
  "migrations=migrations",
]

[module.auth]
enabled = true
path = modules/auth
kind = service
depends = []


[module.realtime]
enabled = true
path = modules/realtime
kind = backend
depends = [
  "auth",
  "notifications",
]

[module.notifications]
enabled = true
path = modules/notifications
kind = service
depends = [
  "auth",
]

[module.workspaces]
enabled = true
path = modules/workspaces
kind = service
depends = [
  "auth",
]

[module.members]
enabled = true
path = modules/members
kind = service
depends = [
  "auth",
  "workspaces",
  "notifications",
]


[module.workspace_invites]
enabled = true
path = modules/workspace_invites
kind = service
depends = [
  "auth",
  "workspaces",
  "members",
  "notifications",
]

[module.projects]
enabled = true
path = modules/projects
kind = service
depends = [
  "auth",
  "workspaces",
]

[module.packages]
enabled = true
path = modules/packages
kind = service
depends = [
  "auth",
  "workspaces",
]

[module.package_versions]
enabled = true
path = modules/package_versions
kind = service
depends = [
  "auth",
  "packages",
  "notifications",
]

[module.tokens]
enabled = true
path = modules/tokens
kind = service
depends = [
  "auth",
  "workspaces",
]

[module.lockfiles]
enabled = true
path = modules/lockfiles
kind = service
depends = [
  "auth",
  "projects",
]

[module.build_reports]
enabled = true
path = modules/build_reports
kind = service
depends = [
  "auth",
  "projects",
  "notifications",
]

[module.feedback]
enabled = true
path = modules/feedback
kind = service
depends = [
  "auth",
  "notifications",
]
