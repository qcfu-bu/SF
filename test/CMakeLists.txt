find_package(Catch2 3 REQUIRED)

add_executable(test test.cpp)
target_compile_features(test PRIVATE cxx_std_23)

target_link_libraries(test PRIVATE
  Catch2::Catch2WithMain
  parsing
  elaborate)
