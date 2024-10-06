add_library(
	shared
	"AtomBasicCodec.cpp"
	"BitTiming.cpp"
	"BlockDecoder.cpp"
	"Compress.cpp"
	"CSWCodec.cpp"
	"CSWCycleDecoder.cpp"
	"CycleDecoder.cpp"
	"FileDecoder.cpp"
	"LevelDecoder.cpp"
	"UEFTapeReader.cpp"
	"WavCycleDecoder.cpp"
	"WavTapeReader.cpp"
	"DataCodec.cpp"
	"FileBlock.cpp"
	"MMCCodec.cpp"	 
	"PcmFile.cpp"
	"TAPCodec.cpp"
	"TapeProperties.cpp"
	"TapeReader.cpp"
	"UEFCodec.cpp"  
	"Utility.cpp"
	"WavEncoder.cpp"
	"zpipe.cpp" 
	"BBMBlockTypes.h" "FileBlock.h"  "TapeReader.h")

# Locate zlib
find_package(ZLIB REQUIRED)

include_directories(
    "${CMAKE_SOURCE_DIR}/shared"
	"${CMAKE_SOURCE_DIR}/gzstream"
	${ZLIB_INCLUDE_DIRS}
)