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
include CMakeFiles/dat2mmc.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/dat2mmc.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/dat2mmc.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/dat2mmc.dir/flags.make

CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o: CMakeFiles/dat2mmc.dir/flags.make
CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o: dat2mmc/dat2mmc.cpp
CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o: CMakeFiles/dat2mmc.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o -MF CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o.d -o CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/dat2mmc/dat2mmc.cpp

CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/dat2mmc/dat2mmc.cpp > CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.i

CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/dat2mmc/dat2mmc.cpp -o CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.s

CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o: CMakeFiles/dat2mmc.dir/flags.make
CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o: dat2mmc/ArgParser.cpp
CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o: CMakeFiles/dat2mmc.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o -MF CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o.d -o CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/dat2mmc/ArgParser.cpp

CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/dat2mmc/ArgParser.cpp > CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.i

CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/dat2mmc/ArgParser.cpp -o CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.s

# Object files for target dat2mmc
dat2mmc_OBJECTS = \
"CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o" \
"CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o"

# External object files for target dat2mmc
dat2mmc_EXTERNAL_OBJECTS =

bin/dat2mmc: CMakeFiles/dat2mmc.dir/dat2mmc/dat2mmc.cpp.o
bin/dat2mmc: CMakeFiles/dat2mmc.dir/dat2mmc/ArgParser.cpp.o
bin/dat2mmc: CMakeFiles/dat2mmc.dir/build.make
bin/dat2mmc: shared/libshared.a
bin/dat2mmc: CMakeFiles/dat2mmc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable bin/dat2mmc"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dat2mmc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/dat2mmc.dir/build: bin/dat2mmc
.PHONY : CMakeFiles/dat2mmc.dir/build

CMakeFiles/dat2mmc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/dat2mmc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/dat2mmc.dir/clean

CMakeFiles/dat2mmc.dir/depend:
	cd /home/joakim/Documents/GitHub/AtomTapeUtils && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles/dat2mmc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/dat2mmc.dir/depend

