cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_package_versions_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_package_versions ${cloud_package_versions_SOURCES})
add_library(cloud::package_versions ALIAS cloud_package_versions)

target_include_directories(cloud_package_versions
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_package_versions PUBLIC cxx_std_20)

target_link_libraries(cloud_package_versions
  PUBLIC
    vix::vix
    cloud_notifications
)

if(DEFINED VIX_MODULE_package_versions_LINKS)
  target_link_libraries(cloud_package_versions
    PUBLIC
      ${VIX_MODULE_package_versions_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_package_versions_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_package_versions_TEST_SOURCES)
    add_executable(cloud_package_versions_tests
      ${cloud_package_versions_TEST_SOURCES}
    )

    target_include_directories(cloud_package_versions_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_package_versions_tests PRIVATE
      cloud_package_versions
      vix::vix
    )

    target_compile_features(cloud_package_versions_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.package_versions COMMAND cloud_package_versions_tests)
  endif()
endif()

set_target_properties(cloud_package_versions PROPERTIES
  OUTPUT_NAME "cloud_package_versions"
)
