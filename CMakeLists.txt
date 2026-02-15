# CMakeList.txt : CMake project for abc2uef, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

project ("AtomTapeUtils" VERSION 2.2)

# Make sure a dynamic library (for ZLIB) is chosen first if available
set(CMAKE_FIND_STATIC LAST)

#option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

# Locate zlib
# If this fails (likely only to fail for Windows), then uncomment the line below
# and set the path to where you installed ZLIB, e.g., "C:/Program Files/zlib"
# Also update the Windows path variable to point to where the DLL is located
# (e.g., C:/Program Files/zlib/bin).
set(ZLIB_ROOT "C:/Program Files/zlib")
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
	mmbutil  "mmbutil/mmbutil.cpp"
	"mmbutil/ArgParser.h"  "mmbutil/ArgParser.cpp")
target_link_libraries(mmbutil PUBLIC shared )

add_executable ( 
	abc2bin  "abc2bin/abc2bin.cpp"
	"abc2bin/ArgParser.h"  "abc2bin/ArgParser.cpp"
)
target_link_libraries(abc2bin PUBLIC shared )

add_executable ( 
	tap2dat  "tap2dat/tap2dat.cpp"
	"tap2dat/ArgParser.h"  "tap2dat/ArgParser.cpp"
)
target_link_libraries(tap2dat PUBLIC shared )

add_executable ( 
	tap2abc  "tap2abc/tap2abc.cpp"
	"tap2abc/ArgParser.h"  "tap2abc/ArgParser.cpp"
)
target_link_libraries(tap2abc PUBLIC shared )

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
	bin2abc  "bin2abc/bin2abc.cpp"
	"bin2abc/ArgParser.h"  "bin2abc/ArgParser.cpp"
)
target_link_libraries(bin2abc PUBLIC shared )

add_executable ( 
	dat2tap  "dat2tap/dat2tap.cpp"
	"dat2tap/ArgParser.h"  "dat2tap/ArgParser.cpp"
)
target_link_libraries(dat2tap PUBLIC shared)

add_executable ( 
	abc2tap  "abc2tap/abc2tap.cpp"
	"abc2tap/ArgParser.h"  "abc2tap/ArgParser.cpp"
)
target_link_libraries(abc2tap PUBLIC shared )

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
	inspectuef  "inspectuef/inspectUEF.cpp"
	"inspectuef/ArgParser.h"  "inspectuef/ArgParser.cpp"
)
target_link_libraries(inspectuef PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	csw2wav  "csw2wav/csw2wav.cpp"
	"csw2wav/ArgParser.h"  "csw2wav/ArgParser.cpp"
)
target_link_libraries(csw2wav PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable ( 
	wav2csw  "wav2csw/wav2csw.cpp"
	"wav2csw/ArgParser.h"  "wav2csw/ArgParser.cpp"
)
target_link_libraries(wav2csw PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)


add_executable ( 
	uef2csw  "uef2csw/uef2csw.cpp"
	"uef2csw/ArgParser.h"  "uef2csw/ArgParser.cpp"
)
target_link_libraries(uef2csw PUBLIC shared gzstream PUBLIC ZLIB::ZLIB)

add_executable ( 
	abc2dat "abc2dat/abc2dat.cpp" 
	"shared/AtomBlockTypes.h" "shared/Logging.h"
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

add_executable(
	ScanTape "ScanTape/ScanTape.cpp"
	"ScanTape/ArgParser.h" "ScanTape/ArgParser.cpp"
)
target_link_libraries(ScanTape PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable(
	ScanDisc "ScanDisc/ScanDisc.cpp"
	"ScanDisc/ArgParser.h" "ScanDisc/ArgParser.cpp"
)
target_link_libraries(ScanDisc PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

add_executable(
	ScanTAP "ScanTAP/ScanTAP.cpp"
	"ScanTAP/ArgParser.h" "ScanTAP/ArgParser.cpp"
)
target_link_libraries(ScanTAP PUBLIC shared gzstream PRIVATE ZLIB::ZLIB)

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

# Installation of mmbutil
install(TARGETS mmbutil DESTINATION bin)
install(
	FILES "mmbutil/ArgParser.h"
    DESTINATION include/mmbutil
)

# Installation of ScanTape
install(TARGETS ScanTape DESTINATION bin)
install(
	FILES "ScanTape/ArgParser.h"
    DESTINATION include/ScanTape	
)

# Installation of abc2dat
install(TARGETS abc2dat DESTINATION bin)
install(
	FILES "abc2dat/ArgParser.h"
    DESTINATION include/abc2dat
)

# Installation of abc2tap
install(TARGETS abc2tap DESTINATION bin)
install(
	FILES "abc2tap/ArgParser.h"
    DESTINATION include/abc2tap
)

# Installation of abc2uef
install(TARGETS abc2uef DESTINATION bin)
install(
	FILES "abc2uef/ArgParser.h"
    DESTINATION include/abc2uef
)

# Installation of abc2wav
install(TARGETS abc2wav DESTINATION bin)
install(
	FILES "abc2wav/ArgParser.h"
    DESTINATION include/abc2wav
)

# Installation of bin2abc
install(TARGETS bin2abc DESTINATION bin)
install(
	FILES "bin2abc/ArgParser.h"
    DESTINATION include/bin2abc
)

# Installation of csw2wav
install(TARGETS csw2wav DESTINATION bin)
install(
	FILES "csw2wav/ArgParser.h"
    DESTINATION include/csw2wav
)

# Installation of dat2abc
install(TARGETS dat2abc DESTINATION bin)
install(
	FILES "dat2abc/ArgParser.h"
    DESTINATION include/dat2abc
)

# Installation of dat2bin
install(TARGETS dat2bin DESTINATION bin)
install(
	FILES "dat2bin/ArgParser.h"
    DESTINATION include/dat2bin
)

# Installation of dat2tap
install(TARGETS dat2tap DESTINATION bin)
install(
	FILES "dat2tap/ArgParser.h"
    DESTINATION include/dat2tap
)

# Installation of dat2uef
install(TARGETS dat2uef DESTINATION bin)
install(
	FILES "dat2uef/ArgParser.h"
    DESTINATION include/dat2uef
)

# Installation of dat2wav
install(TARGETS dat2wav DESTINATION bin)
install(
	FILES "dat2wav/ArgParser.h"
    DESTINATION include/dat2wav
)

# Installation of FilterTape
install(TARGETS FilterTape DESTINATION bin)
install(
	FILES "FilterTape/ArgParser.h"
    DESTINATION include/FilterTape
)

# Installation of inspectfile
install(TARGETS inspectfile DESTINATION bin)
install(
	FILES "inspectfile/ArgParser.h"
    DESTINATION include/inspectfile
)

# Installation of inspectuef
install(TARGETS inspectuef DESTINATION bin)
install(
	FILES "inspectuef/ArgParser.h"
    DESTINATION include/inspectuef
)

# Installation of tap2abc
install(TARGETS tap2abc DESTINATION bin)
install(
	FILES "tap2abc/ArgParser.h"
    DESTINATION include/tap2abc
)

# Installation of tap2dat
install(TARGETS tap2dat DESTINATION bin)
install(
	FILES "tap2dat/ArgParser.h"
    DESTINATION include/tap2dat
)

# Installation of uef2csw
install(TARGETS uef2csw DESTINATION bin)
install(
	FILES "uef2csw/ArgParser.h"
    DESTINATION include/uef2csw
)

# Installation of uef2wav
install(TARGETS uef2wav DESTINATION bin)
install(
	FILES "uef2wav/ArgParser.h"
    DESTINATION include/uef2wav
)

# Installation of wav2csw
install(TARGETS wav2csw DESTINATION bin)
install(
	FILES "wav2csw/ArgParser.h"
    DESTINATION include/wav2csw
)

# Installation of ScanTape
install(TARGETS ScanTape DESTINATION bin)
install(
	FILES "ScanTape/ArgParser.h"
    DESTINATION include/ScanTape
)

# Installation of ScanDisc
install(TARGETS ScanDisc DESTINATION bin)
install(
	FILES "ScanDisc/ArgParser.h"
    DESTINATION include/ScanDisc
)

# Installation of ScanTAP
install(TARGETS ScanTAP DESTINATION bin)
install(
	FILES "ScanTAP/ArgParser.h"
    DESTINATION include/ScanTAP
)

#install(RUNTIME_DEPENDENCIES)


# Installer (will be created if cpack --config CPackConfig.cmake + cpack --config CPackSourceConfig.cmake are run from command line)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}")
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DIRECTORY "distribution")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_NAME "AtomTapeUtils")
set(CPACK_PACKAGE_VERSION_MAJOR "${AtomTapeUtils_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${AtomTapeUtils_VERSION_MINOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Utility to decode Acorn Atom/BBC Micro tapes")
set(CPACK_PACKAGE_DESCRIPTION
"This is a set of utilities mainly targeting decoding and encoding of Acorn Atom & BBC Micro BASIC programs")
IF (WIN32)
  set(CPACK_GENERATOR WIX)
ELSEIF(UNIX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
  #set(CPACK_DEBIAN_PACKAGE_NAME "AtomtapeUtils-dev")
  #set(CPACK_DEBIAN_PACKAGE_VERSION 1.0.0)
  #set(CPACK_DEBIAN_PACKAGE_RELEASE 1)
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Joakim Ohlsson")
ENDIF()
set(CPACK_SOURCE_GENERATOR "TGZ")
include(CPack)

