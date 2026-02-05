add_executable(sf main.cpp)
target_compile_features(sf PRIVATE cxx_std_23)

target_link_libraries(sf PRIVATE 
  parsing
  elaborate)
