find_package(ut REQUIRED)
add_executable(test_vectors test_vectors.cpp)
target_link_libraries(test_vectors Boost::ut tinystd)