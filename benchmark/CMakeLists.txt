# use nanobench for benchmarking
find_package(nanobench REQUIRED)

# Function to create benchmark targets with common settings
function(add_benchmark NAME)
    set(prefix "benchmark_")
    set(target_name "${prefix}${NAME}")
    add_executable(${target_name} "${target_name}.cpp")
    target_link_libraries(${target_name} PRIVATE tinystd nanobench ${ARGN}) # Link to your main project library if needed
endfunction()

add_benchmark(waitfree_spsc_queue)