# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

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
CMAKE_COMMAND = /home/asus/.local/bin/cmake

# The command to remove a file.
RM = /home/asus/.local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/asus/Descargas/cores

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/asus/Descargas/cores

# Include any dependencies generated for this target.
include CMakeFiles/reactor.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/reactor.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/reactor.dir/flags.make

CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.o: CMakeFiles/reactor.dir/flags.make
CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.o: reactor/publisher_server_reactor.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.o -c /home/asus/Descargas/cores/reactor/publisher_server_reactor.cc

CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asus/Descargas/cores/reactor/publisher_server_reactor.cc > CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.i

CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asus/Descargas/cores/reactor/publisher_server_reactor.cc -o CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.s

CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.o: CMakeFiles/reactor.dir/flags.make
CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.o: reactor/subscriber_server_reactor.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.o -c /home/asus/Descargas/cores/reactor/subscriber_server_reactor.cc

CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asus/Descargas/cores/reactor/subscriber_server_reactor.cc > CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.i

CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asus/Descargas/cores/reactor/subscriber_server_reactor.cc -o CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.s

# Object files for target reactor
reactor_OBJECTS = \
"CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.o" \
"CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.o"

# External object files for target reactor
reactor_EXTERNAL_OBJECTS =

libreactor.a: CMakeFiles/reactor.dir/reactor/publisher_server_reactor.cc.o
libreactor.a: CMakeFiles/reactor.dir/reactor/subscriber_server_reactor.cc.o
libreactor.a: CMakeFiles/reactor.dir/build.make
libreactor.a: CMakeFiles/reactor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libreactor.a"
	$(CMAKE_COMMAND) -P CMakeFiles/reactor.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/reactor.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/reactor.dir/build: libreactor.a

.PHONY : CMakeFiles/reactor.dir/build

CMakeFiles/reactor.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/reactor.dir/cmake_clean.cmake
.PHONY : CMakeFiles/reactor.dir/clean

CMakeFiles/reactor.dir/depend:
	cd /home/asus/Descargas/cores && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores/CMakeFiles/reactor.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/reactor.dir/depend
