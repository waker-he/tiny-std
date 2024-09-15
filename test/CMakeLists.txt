# use Boost.UT as unit test framework
find_package(ut REQUIRED)

# Function to create test targets with common settings
function(add_test NAME)
    set(prefix "test_")
    set(target_name "${prefix}${NAME}")
    add_executable(${target_name} "${target_name}.cpp")
    target_link_libraries(${target_name} PRIVATE tinystd Boost::ut) # Link to your main project library if needed
endfunction()

add_test(vectors)
add_test(unique_ptr)
add_test(shared_ptr)
add_test(weak_ptr)
add_test(enable_shared_from_this)
add_test(span)
add_test(waitfree_spsc_queue)
# When using hazard_pointer in implementation of my atomic_shared_ptr, this
# test program suddenly does not compile (reporting some unreadable error
# message regarding the use of Boost) without any modifications to the
# hazard_pointer source file and the test file. It may be an issue of
# the compatibility between C++20 modules and Boost library.
# If we remove the use of hazard_pointer in control_block.cpp, then it
# can compile and run. Now I replace boost::unordered_flat_set with
# std::unordered_set.
add_test(hazard_pointer)
add_test(atomic_shared_ptr)
add_test(any)
