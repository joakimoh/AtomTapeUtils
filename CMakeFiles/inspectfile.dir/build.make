# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/joakim/Documents/GitHub/AtomTapeUtils

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/joakim/Documents/GitHub/AtomTapeUtils

# Include any dependencies generated for this target.
include CMakeFiles/inspectfile.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/inspectfile.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/inspectfile.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/inspectfile.dir/flags.make

CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o: CMakeFiles/inspectfile.dir/flags.make
CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o: inspectfile/InspectFile.cpp
CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o: CMakeFiles/inspectfile.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o -MF CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o.d -o CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/inspectfile/InspectFile.cpp

CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/inspectfile/InspectFile.cpp > CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.i

CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/inspectfile/InspectFile.cpp -o CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.s

CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o: CMakeFiles/inspectfile.dir/flags.make
CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o: inspectfile/ArgParser.cpp
CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o: CMakeFiles/inspectfile.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o -MF CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o.d -o CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/inspectfile/ArgParser.cpp

CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/inspectfile/ArgParser.cpp > CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.i

CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/inspectfile/ArgParser.cpp -o CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.s

# Object files for target inspectfile
inspectfile_OBJECTS = \
"CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o" \
"CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o"

# External object files for target inspectfile
inspectfile_EXTERNAL_OBJECTS =

bin/inspectfile: CMakeFiles/inspectfile.dir/inspectfile/InspectFile.cpp.o
bin/inspectfile: CMakeFiles/inspectfile.dir/inspectfile/ArgParser.cpp.o
bin/inspectfile: CMakeFiles/inspectfile.dir/build.make
bin/inspectfile: CMakeFiles/inspectfile.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable bin/inspectfile"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/inspectfile.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/inspectfile.dir/build: bin/inspectfile
.PHONY : CMakeFiles/inspectfile.dir/build

CMakeFiles/inspectfile.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/inspectfile.dir/cmake_clean.cmake
.PHONY : CMakeFiles/inspectfile.dir/clean

CMakeFiles/inspectfile.dir/depend:
	cd /home/joakim/Documents/GitHub/AtomTapeUtils && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles/inspectfile.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/inspectfile.dir/depend

