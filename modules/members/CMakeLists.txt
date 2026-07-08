cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_members_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_members ${cloud_members_SOURCES})
add_library(cloud::members ALIAS cloud_members)

target_include_directories(cloud_members
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_members PUBLIC cxx_std_20)

target_link_libraries(cloud_members
  PUBLIC
    vix::vix
)

if(DEFINED VIX_MODULE_members_LINKS)
  target_link_libraries(cloud_members
    PUBLIC
      ${VIX_MODULE_members_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_members_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_members_TEST_SOURCES)
    add_executable(cloud_members_tests
      ${cloud_members_TEST_SOURCES}
    )

    target_include_directories(cloud_members_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_members_tests PRIVATE
      cloud_members
      vix::vix
    )

    target_compile_features(cloud_members_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.members COMMAND cloud_members_tests)
  endif()
endif()

set_target_properties(cloud_members PROPERTIES
  OUTPUT_NAME "cloud_members"
)
