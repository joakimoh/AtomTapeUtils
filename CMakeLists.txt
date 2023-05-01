# CMakeList.txt : CMake project for abc2uef, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

project ("AtomTapeUtils")

# Make sure a dynamic library (for ZLIB) is chosen first if available
set(CMAKE_FIND_STATIC LAST)

# Locate zlib
find_package(ZLIB REQUIRED)



# Create sub directory for gzstream library
add_subdirectory(gzstream)

#
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/gzstream"	
	${ZLIB_INCLUDE_DIRS}
)



add_executable ( 
	inspectfile "inspectfile/InspectFile.cpp"
	"inspectfile/ArgParser.h" "inspectfile/ArgParser.cpp"
)


add_executable ( 
	dat2bin  "dat2bin/dat2bin.cpp"
	"dat2bin/ArgParser.h"  "dat2bin/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
)

target_sources(dat2bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)



add_executable ( 
	mmc2abc  "mmc2abc/mmc2abc.cpp"
	"mmc2abc/ArgParser.h"  "mmc2abc/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp"
	"shared/MMCCodec.h" "shared/MMCCodec.cpp"
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
)

target_sources(mmc2abc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(mmc2abc)



add_executable ( 
	mmc2dat  "mmc2dat/mmc2dat.cpp"
	"mmc2dat/ArgParser.h"  "mmc2dat/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp"
	"shared/MMCCodec.h" "shared/MMCCodec.cpp"
)

target_sources(mmc2dat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(mmc2dat)



add_executable ( 
	dat2mmc  "dat2mmc/dat2mmc.cpp"
	"dat2mmc/ArgParser.h"  "dat2mmc/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp"
	"shared/MMCCodec.h" "shared/MMCCodec.cpp"
)

target_sources(dat2mmc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(dat2mmc)



add_executable ( 
	abc2mmc  "abc2mmc/abc2mmc.cpp"
	"abc2mmc/ArgParser.h"  "abc2mmc/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp"
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/MMCCodec.h" "shared/MMCCodec.cpp"
)

target_sources(abc2mmc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(abc2mmc)



add_executable ( 
	dat2uef  "dat2uef/dat2uef.cpp"
	"dat2uef/ArgParser.h"  "dat2uef/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp"
)

target_sources(dat2uef PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(dat2uef PUBLIC gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	uef2wav  "uef2wav/uef2wav.cpp"
	"uef2wav/ArgParser.h"  "uef2wav/ArgParser.cpp"
	"shared/WavEncoder.h" "shared/WavEncoder.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h" "shared/PcmFile.cpp"
)

target_sources(uef2wav PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(uef2wav PUBLIC gzstream PRIVATE ZLIB::ZLIB)



add_executable ( 
	abc2dat "abc2dat/abc2dat.cpp" 
	"shared/BlockTypes.h" "shared/Debug.h"
	"abc2dat/ArgParser.h"  "abc2dat/ArgParser.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h" "shared/PcmFile.cpp"
)

target_sources(abc2dat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)




# Add source to this project's executable.
add_executable ( 
	abc2uef "abc2uef/abc2uef.cpp" "abc2uef/abc2uef.h" 
	"shared/BlockTypes.h"
	"abc2uef/ArgParser.h"  "abc2uef/ArgParser.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h" "shared/PcmFile.cpp"
	"shared/Compress.cpp" "shared/Compress.cpp"
)

target_sources(abc2uef PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(abc2uef PUBLIC gzstream PUBLIC ZLIB::ZLIB)



add_executable ( 
	abc2wav "abc2wav/abc2wav.cpp" 
	"shared/BlockTypes.h"
	"abc2wav/ArgParser.h"  "abc2wav/ArgParser.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/WavEncoder.h" "shared/WavEncoder.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h" "shared/PcmFile.cpp"
)

target_sources(abc2wav PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)



add_executable ( 
	dat2abc "dat2abc/dat2abc.cpp" 
	"dat2abc/ArgParser.h"  "dat2abc/ArgParser.cpp"
	"shared/BlockTypes.h"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h" "shared/PcmFile.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"
)
target_sources(dat2abc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)


add_executable ( 
	dat2wav "dat2wav/dat2wav.cpp" 
	"shared/BlockTypes.h"
	"dat2wav/ArgParser.h"  "dat2wav/ArgParser.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/WavEncoder.h" "shared/WavEncoder.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h" "shared/PcmFile.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"
)

target_sources(dat2wav PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)


add_executable ( 
	uef2abc "uef2abc/uef2abc.cpp" "uef2abc/uef2abc.h" 
	"shared/BlockTypes.h"
	"uef2abc/ArgParser.h"  "uef2abc/ArgParser.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h"  "shared/PcmFile.cpp"
)

target_sources(uef2abc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(uef2abc PUBLIC gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	uef2dat "uef2dat/uef2dat.cpp" 
	"shared/BlockTypes.h"
	"uef2dat/ArgParser.h"  "uef2dat/ArgParser.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/DataCodec.h" "shared/DataCodec.cpp" 
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h"  "shared/PcmFile.cpp"
)

target_sources(uef2dat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(uef2dat PUBLIC gzstream PRIVATE ZLIB::ZLIB)



add_executable(
	ScanTape "ScanTape/ScanTape.cpp"
	"shared/BlockTypes.h" "shared/CommonTypes.h"
	"ScanTape/ArgParser.h" "ScanTape/ArgParser.cpp"
	"ScanTape/LevelDecoder.h" "ScanTape/LevelDecoder.cpp"
	"ScanTape/CycleDecoder.h"
	"ScanTape/CSWCycleDecoder.h" "ScanTape/CSWCycleDecoder.cpp"
	"ScanTape/WavCycleDecoder.h" "ScanTape/WavCycleDecoder.cpp"
	"ScanTape/BlockDecoder.h" "ScanTape/BlockDecoder.cpp"
	"ScanTape/FileDecoder.h" "ScanTape/FileDecoder.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h"  "shared/PcmFile.cpp"
	"shared/MMCCodec.h" "shared/MMCCodec.cpp"
	"shared/CSWCodec.h" "shared/CSWCodec.cpp"
	"shared/zpipe.h" "shared/zpipe.cpp"
)

target_sources(ScanTape PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(ScanTape PUBLIC gzstream PRIVATE ZLIB::ZLIB)



add_executable(
	abc2all "abc2all/abc2all.cpp"
	"shared/BlockTypes.h" "shared/CommonTypes.h"
	"abc2all/ArgParser.h" "abc2all/ArgParser.cpp"
	"shared/DataCodec.h" "shared/DataCodec.cpp"
	"shared/TAPCodec.h" "shared/TAPCodec.cpp" 
	"shared/AtomBasicCodec.h" "shared/AtomBasicCodec.cpp"
	"shared/UEFCodec.h" "shared/UEFCodec.cpp"  
	"shared/Utility.h" "shared/Utility.cpp"
	"shared/TapeProperties.h" "shared/PcmFile.h"  "shared/PcmFile.cpp"
	"shared/MMCCodec.h" "shared/MMCCodec.cpp"
)
target_sources(abc2all PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)

target_link_libraries(abc2all PUBLIC gzstream PRIVATE ZLIB::ZLIB)



add_executable(
	 FilterTape "FilterTape/FilterTape.cpp" "FilterTape/ArgParser.cpp" "FilterTape/ArgParser.h"
	 "shared/PcmFile.h" "shared/CommonTypes.h" "shared/PcmFile.cpp"
	 "FilterTape/Filter.h" "FilterTape/Filter.cpp"
	 "shared/Utility.h" "shared/Utility.cpp"
)
target_sources(FilterTape PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shared)
