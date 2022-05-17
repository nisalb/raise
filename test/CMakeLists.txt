enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/include)

include(GoogleTest)

foreach(module IN LISTS modules)
  add_executable(
    ${module}_test
    ${module}_test.cc
    $<TARGET_OBJECTS:${module}>
  )

  target_link_libraries(
    ${module}_test
    gtest_main
  )
  gtest_discover_tests(${module}_test)
endforeach()
