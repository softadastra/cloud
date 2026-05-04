function(cloud_add_integration_test test_name test_source)
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

cloud_add_integration_test(cloud_migrations_integration_test
  database/MigrationsIntegrationTest.cpp
)

cloud_add_integration_test(cloud_health_routes_integration_test
  health/HealthRoutesIntegrationTest.cpp
)

cloud_add_integration_test(cloud_project_routes_integration_test
  projects/ProjectRoutesIntegrationTest.cpp
)

cloud_add_integration_test(cloud_agent_routes_integration_test
  agents/AgentRoutesIntegrationTest.cpp
)

cloud_add_integration_test(cloud_run_routes_integration_test
  runs/RunRoutesIntegrationTest.cpp
)

cloud_add_integration_test(cloud_event_routes_integration_test
  events/EventRoutesIntegrationTest.cpp
)

cloud_add_integration_test(cloud_report_routes_integration_test
  reports/ReportRoutesIntegrationTest.cpp
)

cloud_add_integration_test(cloud_full_reliability_flow_integration_test
  reliability/FullReliabilityFlowIntegrationTest.cpp
)
