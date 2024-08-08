add_library(tinystd)
target_sources(tinystd
  PUBLIC
    FILE_SET CXX_MODULES FILES
      tinystd.cppm
)