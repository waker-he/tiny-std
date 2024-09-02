add_library(tinystd)
target_sources(tinystd
  PUBLIC
    FILE_SET CXX_MODULES FILES
      tinystd.cppm type_traits.cppm
      vectors/vector_mixin.cpp
      vectors/small_vector.cppm
      vectors/vector.cppm
      vectors/inplace_vector.cppm
      helpers/manual_lifetime.cpp
      smart_pointers/unique_ptr.cppm
      smart_pointers/shared_ptr.cppm
      smart_pointers/control_block.cpp
      smart_pointers/weak_ptr.cppm
      smart_pointers/enable_shared_from_this.cppm
      span.cppm
      waitfree_spsc_queue.cppm
)