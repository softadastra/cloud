cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_tokens_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_tokens ${cloud_tokens_SOURCES})
add_library(cloud::tokens ALIAS cloud_tokens)

target_include_directories(cloud_tokens
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_tokens PUBLIC cxx_std_20)

target_link_libraries(cloud_tokens
  PUBLIC
    vix::vix
)

if(DEFINED VIX_MODULE_tokens_LINKS)
  target_link_libraries(cloud_tokens
    PUBLIC
      ${VIX_MODULE_tokens_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_tokens_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_tokens_TEST_SOURCES)
    add_executable(cloud_tokens_tests
      ${cloud_tokens_TEST_SOURCES}
    )

    target_include_directories(cloud_tokens_tests PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${CMAKE_CURRENT_LIST_DIR}/src
    )

    target_link_libraries(cloud_tokens_tests PRIVATE
      cloud_tokens
      vix::vix
    )

    target_compile_features(cloud_tokens_tests PRIVATE cxx_std_20)

    add_test(NAME cloud.tokens COMMAND cloud_tokens_tests)
  endif()
endif()

set_target_properties(cloud_tokens PROPERTIES
  OUTPUT_NAME "cloud_tokens"
)
