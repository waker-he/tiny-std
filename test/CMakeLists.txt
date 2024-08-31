find_package(ut REQUIRED)

add_executable(test_vectors test_vectors.cpp)
target_link_libraries(test_vectors Boost::ut tinystd)

add_executable(test_unique_ptr test_unique_ptr.cpp)
target_link_libraries(test_unique_ptr tinystd)

add_executable(test_shared_ptr test_shared_ptr.cpp)
target_link_libraries(test_shared_ptr Boost::ut tinystd)

add_executable(test_weak_ptr test_weak_ptr.cpp)
target_link_libraries(test_weak_ptr Boost::ut tinystd)