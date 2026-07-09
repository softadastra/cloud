cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_realtime_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_realtime ${cloud_realtime_SOURCES})
add_library(cloud::realtime ALIAS cloud_realtime)

target_include_directories(cloud_realtime
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_realtime PUBLIC cxx_std_20)

target_link_libraries(cloud_realtime
  PUBLIC
    vix::vix
    vix::websocket
    cloud_auth
    cloud_notifications
)

if(DEFINED VIX_MODULE_realtime_LINKS)
  target_link_libraries(cloud_realtime
    PUBLIC
      ${VIX_MODULE_realtime_LINKS}
  )
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_realtime_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
  )

  if(cloud_realtime_TEST_SOURCES)
    add_executable(cloud_realtime_tests ${cloud_realtime_TEST_SOURCES})
    target_link_libraries(cloud_realtime_tests PRIVATE cloud_realtime vix::vix)
    target_compile_features(cloud_realtime_tests PRIVATE cxx_std_20)
    add_test(NAME cloud.realtime COMMAND cloud_realtime_tests)
  endif()
endif()

set_target_properties(cloud_realtime PROPERTIES
  OUTPUT_NAME "cloud_realtime"
)
