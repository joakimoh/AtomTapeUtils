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
include CMakeFiles/uef2abc.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/uef2abc.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/uef2abc.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/uef2abc.dir/flags.make

CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o: CMakeFiles/uef2abc.dir/flags.make
CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o: uef2abc/uef2abc.cpp
CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o: CMakeFiles/uef2abc.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o -MF CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o.d -o CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/uef2abc/uef2abc.cpp

CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/uef2abc/uef2abc.cpp > CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.i

CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/uef2abc/uef2abc.cpp -o CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.s

CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o: CMakeFiles/uef2abc.dir/flags.make
CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o: uef2abc/ArgParser.cpp
CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o: CMakeFiles/uef2abc.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o -MF CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o.d -o CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o -c /home/joakim/Documents/GitHub/AtomTapeUtils/uef2abc/ArgParser.cpp

CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/joakim/Documents/GitHub/AtomTapeUtils/uef2abc/ArgParser.cpp > CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.i

CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/joakim/Documents/GitHub/AtomTapeUtils/uef2abc/ArgParser.cpp -o CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.s

# Object files for target uef2abc
uef2abc_OBJECTS = \
"CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o" \
"CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o"

# External object files for target uef2abc
uef2abc_EXTERNAL_OBJECTS =

bin/uef2abc: CMakeFiles/uef2abc.dir/uef2abc/uef2abc.cpp.o
bin/uef2abc: CMakeFiles/uef2abc.dir/uef2abc/ArgParser.cpp.o
bin/uef2abc: CMakeFiles/uef2abc.dir/build.make
bin/uef2abc: shared/libshared.a
bin/uef2abc: gzstream/libgzstream.a
bin/uef2abc: /usr/lib/x86_64-linux-gnu/libz.so
bin/uef2abc: CMakeFiles/uef2abc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable bin/uef2abc"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/uef2abc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/uef2abc.dir/build: bin/uef2abc
.PHONY : CMakeFiles/uef2abc.dir/build

CMakeFiles/uef2abc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/uef2abc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/uef2abc.dir/clean

CMakeFiles/uef2abc.dir/depend:
	cd /home/joakim/Documents/GitHub/AtomTapeUtils && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils /home/joakim/Documents/GitHub/AtomTapeUtils/CMakeFiles/uef2abc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/uef2abc.dir/depend
