add_compile_options(-Wall -Werror -fpie -Wl,-pie)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(libraise SHARED)

foreach(module IN LISTS modules)
  add_library(${module} OBJECT ${module}.c)
  target_sources(libraise PRIVATE ${module}.c)
endforeach()
