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
add_test(hazard_pointer)
