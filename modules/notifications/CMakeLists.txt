cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE cloud_notifications_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

add_library(cloud_notifications ${cloud_notifications_SOURCES})
add_library(cloud::notifications ALIAS cloud_notifications)

target_include_directories(cloud_notifications
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_features(cloud_notifications PUBLIC cxx_std_20)

target_link_libraries(cloud_notifications PUBLIC vix::vix cloud_auth)

if(DEFINED VIX_MODULE_notifications_LINKS)
  target_link_libraries(cloud_notifications PUBLIC ${VIX_MODULE_notifications_LINKS})
endif()

if(DEFINED cloud_BUILD_TESTS AND cloud_BUILD_TESTS)
  file(GLOB_RECURSE cloud_notifications_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp)
  if(cloud_notifications_TEST_SOURCES)
    add_executable(cloud_notifications_tests ${cloud_notifications_TEST_SOURCES})
    target_link_libraries(cloud_notifications_tests PRIVATE cloud_notifications vix::vix)
    target_compile_features(cloud_notifications_tests PRIVATE cxx_std_20)
    add_test(NAME cloud.notifications COMMAND cloud_notifications_tests)
  endif()
endif()

set_target_properties(cloud_notifications PROPERTIES OUTPUT_NAME "cloud_notifications")
