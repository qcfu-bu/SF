add_library(parsing
  syntax.cpp
  lexer.cpp
  parser.cpp)
target_compile_features(parsing PRIVATE cxx_std_23)

target_include_directories(parsing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)