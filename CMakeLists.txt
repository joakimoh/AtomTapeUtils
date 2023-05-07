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

# Create sub driectory for shared library
add_subdirectory(shared)

#
include_directories(
    "${PROJECT_BINARY_DIR}"
    "${CMAKE_SOURCE_DIR}/gzstream"
	"${CMAKE_SOURCE_DIR}/shared"	
	${ZLIB_INCLUDE_DIRS}
)

add_executable ( 
	inspectfile "inspectfile/InspectFile.cpp"
	"inspectfile/ArgParser.h" "inspectfile/ArgParser.cpp"
)

add_executable ( 
	dat2bin  "dat2bin/dat2bin.cpp"
	"dat2bin/ArgParser.h"  "dat2bin/ArgParser.cpp"
)
target_link_libraries(dat2bin PUBLIC shared )

add_executable ( 
	mmc2abc  "mmc2abc/mmc2abc.cpp"
	"mmc2abc/ArgParser.h"  "mmc2abc/ArgParser.cpp"
)
target_link_libraries(mmc2abc PUBLIC shared)

add_executable ( 
	mmc2dat  "mmc2dat/mmc2dat.cpp"
	"mmc2dat/ArgParser.h"  "mmc2dat/ArgParser.cpp"
)
target_link_libraries(mmc2dat PUBLIC shared)

add_executable ( 
	dat2mmc  "dat2mmc/dat2mmc.cpp"
	"dat2mmc/ArgParser.h"  "dat2mmc/ArgParser.cpp"
)
target_link_libraries(dat2mmc PUBLIC shared)

add_executable ( 
	abc2mmc  "abc2mmc/abc2mmc.cpp"
	"abc2mmc/ArgParser.h"  "abc2mmc/ArgParser.cpp"
)
target_link_libraries(abc2mmc PUBLIC shared )

add_executable ( 
	dat2uef  "dat2uef/dat2uef.cpp"
	"dat2uef/ArgParser.h"  "dat2uef/ArgParser.cpp"
)
target_link_libraries(dat2uef PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	uef2wav  "uef2wav/uef2wav.cpp"
	"uef2wav/ArgParser.h"  "uef2wav/ArgParser.cpp"
)
target_link_libraries(uef2wav PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	csw2wav  "csw2wav/csw2wav.cpp"
	"csw2wav/ArgParser.h"  "csw2wav/ArgParser.cpp"
)
target_link_libraries(csw2wav PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)


add_executable ( 
	uef2csw  "uef2csw/uef2csw.cpp"
	"uef2csw/ArgParser.h"  "uef2csw/ArgParser.cpp"
)
target_link_libraries(uef2csw PUBLIC shared gzstream PUBLIC ZLIB::ZLIB)

add_executable ( 
	abc2dat "abc2dat/abc2dat.cpp" 
	"shared/BlockTypes.h" "shared/Debug.h"
	"abc2dat/ArgParser.h"  "abc2dat/ArgParser.cpp"
)
target_link_libraries(abc2dat PUBLIC shared)

# Add source to this project's executable.
add_executable ( 
	abc2uef "abc2uef/abc2uef.cpp"
	"abc2uef/ArgParser.h"  "abc2uef/ArgParser.cpp"
)
target_link_libraries(abc2uef PUBLIC shared gzstream PUBLIC ZLIB::ZLIB)

add_executable ( 
	abc2wav "abc2wav/abc2wav.cpp" 
	"abc2wav/ArgParser.h"  "abc2wav/ArgParser.cpp"
)
target_link_libraries(abc2wav PUBLIC shared)


add_executable ( 
	dat2abc "dat2abc/dat2abc.cpp" 
	"dat2abc/ArgParser.h"  "dat2abc/ArgParser.cpp"
)
target_link_libraries(dat2abc PUBLIC shared)

add_executable ( 
	dat2wav "dat2wav/dat2wav.cpp" 
	"dat2wav/ArgParser.h"  "dat2wav/ArgParser.cpp"
)
target_link_libraries(dat2wav PUBLIC shared)

add_executable ( 
	uef2abc "uef2abc/uef2abc.cpp"
	"uef2abc/ArgParser.h"  "uef2abc/ArgParser.cpp"
)
target_link_libraries(uef2abc PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	uef2dat "uef2dat/uef2dat.cpp" 
	"uef2dat/ArgParser.h"  "uef2dat/ArgParser.cpp"
)
target_link_libraries(uef2dat PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable(
	ScanTape "ScanTape/ScanTape.cpp"
	"ScanTape/ArgParser.h" "ScanTape/ArgParser.cpp"
	"ScanTape/LevelDecoder.h" "ScanTape/LevelDecoder.cpp"
	"ScanTape/CycleDecoder.h"
	"ScanTape/CSWCycleDecoder.h" "ScanTape/CSWCycleDecoder.cpp"
	"ScanTape/WavCycleDecoder.h" "ScanTape/WavCycleDecoder.cpp"
	"ScanTape/BlockDecoder.h" "ScanTape/BlockDecoder.cpp"
	"ScanTape/FileDecoder.h" "ScanTape/FileDecoder.cpp"
)
target_link_libraries(ScanTape PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable(
	abc2all "abc2all/abc2all.cpp"
	"abc2all/ArgParser.h" "abc2all/ArgParser.cpp"
)
target_link_libraries(abc2all PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable(
	 FilterTape "FilterTape/FilterTape.cpp" "FilterTape/ArgParser.cpp" "FilterTape/ArgParser.h"
	 "FilterTape/Filter.h" "FilterTape/Filter.cpp"
)
target_link_libraries(FilterTape PUBLIC shared)
