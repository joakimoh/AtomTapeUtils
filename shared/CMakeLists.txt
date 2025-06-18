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
	"PcmFile.cpp"
	"TAPCodec.cpp"
	"TapeProperties.cpp"
	"TapeReader.cpp"
	"UEFCodec.cpp"  
	"Utility.cpp"
	"WavEncoder.cpp"
	"zpipe.cpp" 
	"BBMBlockTypes.h" "FileBlock.h"  "TapeReader.h" "DiscCodec.h" "DiscCodec.cpp" "BinCodec.cpp" "BinCodec.h" "MMBCodec.cpp" "MMBCodec.h")

# Locate zlib
find_package(ZLIB REQUIRED)

include_directories(
    "${CMAKE_SOURCE_DIR}/shared"
	"${CMAKE_SOURCE_DIR}/gzstream"
	${ZLIB_INCLUDE_DIRS}
)

# Install information - lib & include
set(installable_libs shared)
install(TARGETS ${installable_libs} DESTINATION lib)
install(
	FILES AtomBasicCodec.h AtomBlockTypes.h BBMBlockTypes.h BinCodec.h BlockDecoder.h
	CommonTypes.h Compress.h CSWCodec.h CSWCycleDecoder.h CycleDecoder.h DataCodec.h DiscCodec.h
	FileBlock.h FileDecoder.h LevelDecoder.h Logging.h PcmFile.h TAPCodec.h
	TapeProperties.h TapeReader.h UEFCodec.h UEFTapeReader.h Utility.h
	WavCycleDecoder.h WavEncoder.h WaveSampleTypes.h WavTapeReader.h zpipe.h
	DESTINATION include/shared
)
