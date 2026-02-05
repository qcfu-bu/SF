add_library(elaborate
  table.cpp
  elab.cpp)
target_compile_features(elaborate PRIVATE cxx_std_23)

target_include_directories(elaborate PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)