add_library(gzstream gzstream.cpp)

# Locate zlib
find_package(ZLIB REQUIRED)

include_directories(
    "${CMAKE_SOURCE_DIR}/gzstream"
	${ZLIB_INCLUDE_DIRS}
)