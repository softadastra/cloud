cmake_minimum_required(VERSION 3.20)

project(softadastra_cloud LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SOFTADASTRA_CLOUD_BUILD_TESTS "Build Softadastra Cloud tests" ON)
option(SOFTADASTRA_CLOUD_LINK_STATIC "Static libstdc++/libgcc" OFF)
option(SOFTADASTRA_CLOUD_LINK_FULL_STATIC "Full static link (-static)" OFF)

# ------------------------------------------------------
# Vix runtime
# ------------------------------------------------------
find_package(vix QUIET CONFIG)

if(NOT vix_FOUND)
  find_package(Vix CONFIG REQUIRED)
endif()

# ------------------------------------------------------
# Optional local Vix registry packages
# ------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.vix/vix_deps.cmake")
  include("${CMAKE_CURRENT_SOURCE_DIR}/.vix/vix_deps.cmake")
endif()

# ------------------------------------------------------
# Helpers
# ------------------------------------------------------
function(cloud_apply_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

function(cloud_apply_static_link_flags target_name)
  if(MSVC)
    return()
  endif()

  if(SOFTADASTRA_CLOUD_LINK_FULL_STATIC)
    target_link_options(${target_name} PRIVATE -static)
    target_compile_definitions(${target_name} PRIVATE SOFTADASTRA_CLOUD_LINK_FULL_STATIC=1)
  elseif(SOFTADASTRA_CLOUD_LINK_STATIC)
    target_link_options(${target_name} PRIVATE -static-libstdc++ -static-libgcc)
    target_compile_definitions(${target_name} PRIVATE SOFTADASTRA_CLOUD_LINK_STATIC=1)
  endif()
endfunction()

function(cloud_configure_target target_name)
  target_include_directories(${target_name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(${target_name}
    PRIVATE
      vix::vix
  )

  cloud_apply_warnings(${target_name})
  cloud_apply_static_link_flags(${target_name})
endfunction()

# ------------------------------------------------------
# Core backend sources
# ------------------------------------------------------
set(SOFTADASTRA_CLOUD_SOURCES
  src/app/AppFactory.cpp

  src/config/CloudConfig.cpp

  src/database/Database.cpp
  src/database/Migrations.cpp

  src/http/JsonResponse.cpp
  src/http/RequestContext.cpp

  src/middlewares/MiddlewareRegistry.cpp
  src/middlewares/RequestIdMiddleware.cpp
  src/middlewares/AgentAuthMiddleware.cpp

  src/modules/health/HealthController.cpp
  src/modules/health/HealthRoutes.cpp

  src/modules/auth/AuthController.cpp
  src/modules/auth/AuthService.cpp
  src/modules/auth/AuthRepository.cpp
  src/modules/auth/AuthRoutes.cpp

  src/modules/projects/ProjectController.cpp
  src/modules/projects/ProjectService.cpp
  src/modules/projects/ProjectRepository.cpp
  src/modules/projects/ProjectRoutes.cpp

  src/modules/agents/AgentController.cpp
  src/modules/agents/AgentService.cpp
  src/modules/agents/AgentRepository.cpp
  src/modules/agents/AgentRoutes.cpp

  src/modules/runs/RunController.cpp
  src/modules/runs/RunService.cpp
  src/modules/runs/RunRepository.cpp
  src/modules/runs/RunRoutes.cpp

  src/modules/events/EventController.cpp
  src/modules/events/EventService.cpp
  src/modules/events/EventRepository.cpp
  src/modules/events/EventRoutes.cpp

  src/modules/reports/ReportController.cpp
  src/modules/reports/ReportService.cpp
  src/modules/reports/ReportRepository.cpp
  src/modules/reports/ReportRoutes.cpp

  src/modules/scoring/ScoringService.cpp

  src/security/ApiKeyHasher.cpp
  src/security/TokenService.cpp

  src/validation/RunValidator.cpp
  src/validation/EventValidator.cpp

  src/utils/Time.cpp
  src/utils/IdGenerator.cpp
)

# ------------------------------------------------------
# Core library
# ------------------------------------------------------
add_library(softadastra_cloud_core STATIC
  ${SOFTADASTRA_CLOUD_SOURCES}
)

cloud_configure_target(softadastra_cloud_core)

# ------------------------------------------------------
# Main executable
# ------------------------------------------------------
add_executable(cloud
  src/main.cpp
)

target_link_libraries(cloud
  PRIVATE
    softadastra_cloud_core
)

cloud_configure_target(cloud)

# ------------------------------------------------------
# Tests
# ------------------------------------------------------
if(SOFTADASTRA_CLOUD_BUILD_TESTS)
  include(CTest)
  enable_testing()

  add_subdirectory(tests/unit)
  add_subdirectory(tests/integration)
endif()

# ------------------------------------------------------
# Convenience target
# ------------------------------------------------------
add_custom_target(run
  COMMAND $<TARGET_FILE:cloud>
  DEPENDS cloud
  USES_TERMINAL
)
