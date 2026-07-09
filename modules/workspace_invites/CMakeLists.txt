cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_workspace_invites_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_workspace_invites ${cloud_workspace_invites_SOURCES})
add_library(cloud::workspace_invites ALIAS cloud_workspace_invites)

target_include_directories(cloud_workspace_invites
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_workspace_invites PUBLIC cxx_std_20)

target_link_libraries(cloud_workspace_invites
  PUBLIC
    vix::vix
    cloud_notifications
)

if(DEFINED VIX_MODULE_workspace_invites_LINKS)
  target_link_libraries(cloud_workspace_invites
    PUBLIC
      ${VIX_MODULE_workspace_invites_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_workspace_invites_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_workspace_invites_TEST_SOURCES)
    add_executable(cloud_workspace_invites_tests
      ${cloud_workspace_invites_TEST_SOURCES}
    )

    target_include_directories(cloud_workspace_invites_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_workspace_invites_tests PRIVATE
      cloud_workspace_invites
      vix::vix
    )

    target_compile_features(cloud_workspace_invites_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.workspace_invites COMMAND cloud_workspace_invites_tests)
  endif()
endif()

set_target_properties(cloud_workspace_invites PROPERTIES
  OUTPUT_NAME "cloud_workspace_invites"
)
