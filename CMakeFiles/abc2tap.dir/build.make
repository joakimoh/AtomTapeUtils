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
include CMakeFiles/abc2tap.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/abc2tap.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/abc2tap.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/abc2tap.dir/flags.make

CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o: CMakeFiles/abc2tap.dir/flags.make
CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o: abc2tap/abc2tap.cpp
CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o: CMakeFiles/abc2tap.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o -MF CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o.d -o CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/abc2tap/abc2tap.cpp

CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/abc2tap/abc2tap.cpp > CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.i

CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/abc2tap/abc2tap.cpp -o CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.s

CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o: CMakeFiles/abc2tap.dir/flags.make
CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o: abc2tap/ArgParser.cpp
CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o: CMakeFiles/abc2tap.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o -MF CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o.d -o CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/abc2tap/ArgParser.cpp

CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/abc2tap/ArgParser.cpp > CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.i

CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/abc2tap/ArgParser.cpp -o CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.s

# Object files for target abc2tap
abc2tap_OBJECTS = \
"CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o" \
"CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o"

# External object files for target abc2tap
abc2tap_EXTERNAL_OBJECTS =

bin/abc2tap: CMakeFiles/abc2tap.dir/abc2tap/abc2tap.cpp.o
bin/abc2tap: CMakeFiles/abc2tap.dir/abc2tap/ArgParser.cpp.o
bin/abc2tap: CMakeFiles/abc2tap.dir/build.make
bin/abc2tap: shared/libshared.a
bin/abc2tap: CMakeFiles/abc2tap.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable bin/abc2tap"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/abc2tap.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/abc2tap.dir/build: bin/abc2tap
.PHONY : CMakeFiles/abc2tap.dir/build

CMakeFiles/abc2tap.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/abc2tap.dir/cmake_clean.cmake
.PHONY : CMakeFiles/abc2tap.dir/clean

CMakeFiles/abc2tap.dir/depend:
	cd /home/joakim/Documents/GitHub/AtomTapeUtils && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles/abc2tap.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/abc2tap.dir/depend

