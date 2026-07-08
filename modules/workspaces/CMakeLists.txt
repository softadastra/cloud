cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_workspaces_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_workspaces ${cloud_workspaces_SOURCES})
add_library(cloud::workspaces ALIAS cloud_workspaces)

target_include_directories(cloud_workspaces
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_workspaces PUBLIC cxx_std_20)

target_link_libraries(cloud_workspaces
  PUBLIC
    vix::vix
)

if(DEFINED VIX_MODULE_workspaces_LINKS)
  target_link_libraries(cloud_workspaces
    PUBLIC
      ${VIX_MODULE_workspaces_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_workspaces_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_workspaces_TEST_SOURCES)
    add_executable(cloud_workspaces_tests
      ${cloud_workspaces_TEST_SOURCES}
    )

    target_include_directories(cloud_workspaces_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_workspaces_tests PRIVATE
      cloud_workspaces
      vix::vix
    )

    target_compile_features(cloud_workspaces_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.workspaces COMMAND cloud_workspaces_tests)
  endif()
endif()

set_target_properties(cloud_workspaces PROPERTIES
  OUTPUT_NAME "cloud_workspaces"
)
