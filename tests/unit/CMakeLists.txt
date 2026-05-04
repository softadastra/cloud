function(cloud_add_unit_test test_name test_source)
  add_executable(${test_name}
    ${test_source}
  )

  target_link_libraries(${test_name}
    PRIVATE
      softadastra_cloud_core
  )

  cloud_configure_target(${test_name})

  add_test(
    NAME ${test_name}
    COMMAND ${test_name}
  )
endfunction()

cloud_add_unit_test(cloud_time_test
  utils/TimeTest.cpp
)

cloud_add_unit_test(cloud_id_generator_test
  utils/IdGeneratorTest.cpp
)

cloud_add_unit_test(cloud_run_validator_test
  validation/RunValidatorTest.cpp
)

cloud_add_unit_test(cloud_event_validator_test
  validation/EventValidatorTest.cpp
)

cloud_add_unit_test(cloud_api_key_hasher_test
  security/ApiKeyHasherTest.cpp
)

cloud_add_unit_test(cloud_token_service_test
  security/TokenServiceTest.cpp
)

cloud_add_unit_test(cloud_request_context_test
  http/RequestContextTest.cpp
)

cloud_add_unit_test(cloud_json_response_test
  http/JsonResponseTest.cpp
)

cloud_add_unit_test(cloud_config_test
  config/CloudConfigTest.cpp
)

cloud_add_unit_test(cloud_project_service_test
  projects/ProjectServiceTest.cpp
)

cloud_add_unit_test(cloud_agent_service_test
  agents/AgentServiceTest.cpp
)

cloud_add_unit_test(cloud_run_service_test
  runs/RunServiceTest.cpp
)

cloud_add_unit_test(cloud_event_service_test
  events/EventServiceTest.cpp
)

cloud_add_unit_test(cloud_report_service_test
  reports/ReportServiceTest.cpp
)

cloud_add_unit_test(cloud_scoring_service_test
  scoring/ScoringServiceTest.cpp
)
