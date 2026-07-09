add_library(cloud_feedback STATIC
  src/FeedbackModule.cpp
  src/controllers/FeedbackController.cpp
  src/services/FeedbackService.cpp
)

add_library(cloud::feedback ALIAS cloud_feedback)

target_include_directories(cloud_feedback
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cloud_feedback
  PUBLIC
    vix::vix
    cloud_auth
    cloud_notifications
)


if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
