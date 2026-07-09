cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_build_reports_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_build_reports ${cloud_build_reports_SOURCES})
add_library(cloud::build_reports ALIAS cloud_build_reports)

target_include_directories(cloud_build_reports
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_build_reports PUBLIC cxx_std_20)

target_link_libraries(cloud_build_reports
  PUBLIC
    vix::vix
    cloud_notifications
)

if(DEFINED VIX_MODULE_build_reports_LINKS)
  target_link_libraries(cloud_build_reports
    PUBLIC
      ${VIX_MODULE_build_reports_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_build_reports_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_build_reports_TEST_SOURCES)
    add_executable(cloud_build_reports_tests
      ${cloud_build_reports_TEST_SOURCES}
    )

    target_include_directories(cloud_build_reports_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_build_reports_tests PRIVATE
      cloud_build_reports
      vix::vix
    )

    target_compile_features(cloud_build_reports_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.build_reports COMMAND cloud_build_reports_tests)
  endif()
endif()

set_target_properties(cloud_build_reports PROPERTIES
  OUTPUT_NAME "cloud_build_reports"
)
