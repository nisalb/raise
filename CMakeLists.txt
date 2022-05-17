cmake_minimum_required(VERSION 3.23)
project(
  raise
  VERSION 0.1
  DESCRIPTION "A C library of common data structures and algorithms"
  LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# for ccls
set(CMAKE_EXPORT_COMPILE_COMMANDS Yes)

include(FetchContent)
fetchcontent_declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929
)
fetchcontent_makeavailable(googletest)

find_package(Doxygen)

set(DOXYGEN_GENERATE_HTML Yes)
set(DOXYGEN_GENERATE_MAN No)
set(DOXYGEN_GENERATE_LATEX No)
set(DOXYGEN_PROJECT_NAME "Raise")
set(DOXYGEN_PROJECT_BRIEF ${PROJECT_DESCRIPTION})
set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/docs/build)
set(DOXYGEN_HTML_HEADER ${PROJECT_SOURCE_DIR}/docs/header.html)
set(
  DOXYGEN_HTML_EXTRA_STYLESHEET
  ${PROJECT_SOURCE_DIR}/extra/doxygen-awesome-css/doxygen-awesome.css
)
set(
  DOXYGEN_HTML_EXTRA_FILES
  ${PROJECT_SOURCE_DIR}/extra/doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js
  ${PROJECT_SOURCE_DIR}/extra/doxygen-awesome-css/doxygen-awesome-paragraph-link.js
)
set(DOXYGEN_JAVADOC_AUTOBRIEF Yes)
set(DOXYGEN_GENERATE_TREEVIEW Yes)

doxygen_add_docs(
  docs
  ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/docs
  ALL
  COMMENT "Generate HTML Docs"
)

set(modules vector)
add_subdirectory(src)
add_subdirectory(test)

add_custom_target(
  update_compile_commands ALL
  ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_CURRENT_LIST_DIR}
)
