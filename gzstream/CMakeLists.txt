add_library(gzstream gzstream.cpp)

include_directories(
    "${CMAKE_SOURCE_DIR}/gzstream"
	"${CMAKE_SOURCE_DIR}/zlib"					  
)