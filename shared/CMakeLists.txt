add_library(
	shared
	"AtomBasicCodec.cpp"
	"Compress.cpp"
	"CSWCodec.cpp"
	"DataCodec.cpp"
	"MMCCodec.cpp"	 
	"PcmFile.cpp"
	"TAPCodec.cpp"
	"UEFCodec.cpp"  
	"Utility.cpp"
	"WavEncoder.cpp"
	"zpipe.cpp" "BBMBlockTypes.h" "BlockTypes.h")

# Locate zlib
find_package(ZLIB REQUIRED)

include_directories(
    "${CMAKE_SOURCE_DIR}/shared"
	"${CMAKE_SOURCE_DIR}/gzstream"
	${ZLIB_INCLUDE_DIRS}
)