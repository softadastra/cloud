cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_auth_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_auth ${cloud_auth_SOURCES})
add_library(cloud::auth ALIAS cloud_auth)

target_include_directories(cloud_auth
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_auth PUBLIC cxx_std_20)

target_link_libraries(cloud_auth
  PUBLIC
    vix::vix
)

if(DEFINED VIX_MODULE_auth_LINKS)
  target_link_libraries(cloud_auth
    PUBLIC
      ${VIX_MODULE_auth_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_auth_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_auth_TEST_SOURCES)
    add_executable(cloud_auth_tests
      ${cloud_auth_TEST_SOURCES}
    )

    target_include_directories(cloud_auth_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_auth_tests PRIVATE
      cloud_auth
      vix::vix
    )

    target_compile_features(cloud_auth_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.auth COMMAND cloud_auth_tests)
  endif()
endif()

set_target_properties(cloud_auth PROPERTIES
  OUTPUT_NAME "cloud_auth"
)
