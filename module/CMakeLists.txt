add_library(tinystd)
target_sources(tinystd
  PUBLIC
    FILE_SET CXX_MODULES FILES
      tinystd.cppm type_traits.cppm
      vectors/vector_mixin.cpp
      vectors/small_vector.cppm
      vectors/vector.cppm
      vectors/inplace_vector.cppm
)