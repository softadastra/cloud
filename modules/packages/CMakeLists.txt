cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_packages_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_packages ${cloud_packages_SOURCES})
add_library(cloud::packages ALIAS cloud_packages)

target_include_directories(cloud_packages
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_packages PUBLIC cxx_std_20)

target_link_libraries(cloud_packages
  PUBLIC
    vix::vix
)

if(DEFINED VIX_MODULE_packages_LINKS)
  target_link_libraries(cloud_packages
    PUBLIC
      ${VIX_MODULE_packages_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_packages_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_packages_TEST_SOURCES)
    add_executable(cloud_packages_tests
      ${cloud_packages_TEST_SOURCES}
    )

    target_include_directories(cloud_packages_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_packages_tests PRIVATE
      cloud_packages
      vix::vix
    )

    target_compile_features(cloud_packages_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.packages COMMAND cloud_packages_tests)
  endif()
endif()

set_target_properties(cloud_packages PROPERTIES
  OUTPUT_NAME "cloud_packages"
)
