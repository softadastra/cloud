cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_lockfiles_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_lockfiles ${cloud_lockfiles_SOURCES})
add_library(cloud::lockfiles ALIAS cloud_lockfiles)

target_include_directories(cloud_lockfiles
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_lockfiles PUBLIC cxx_std_20)

target_link_libraries(cloud_lockfiles
  PUBLIC
    vix::vix
)

if(DEFINED VIX_MODULE_lockfiles_LINKS)
  target_link_libraries(cloud_lockfiles
    PUBLIC
      ${VIX_MODULE_lockfiles_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_lockfiles_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_lockfiles_TEST_SOURCES)
    add_executable(cloud_lockfiles_tests
      ${cloud_lockfiles_TEST_SOURCES}
    )

    target_include_directories(cloud_lockfiles_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_lockfiles_tests PRIVATE
      cloud_lockfiles
      vix::vix
    )

    target_compile_features(cloud_lockfiles_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.lockfiles COMMAND cloud_lockfiles_tests)
  endif()
endif()

set_target_properties(cloud_lockfiles PROPERTIES
  OUTPUT_NAME "cloud_lockfiles"
)
