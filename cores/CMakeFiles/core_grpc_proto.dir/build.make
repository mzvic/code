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
include CMakeFiles/core_grpc_proto.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/core_grpc_proto.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/core_grpc_proto.dir/flags.make

core.pb.cc: core.proto
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating core.pb.cc, core.pb.h, core.grpc.pb.cc, core.grpc.pb.h"
	/home/asus/.local/bin/protoc-3.21.6.0 --grpc_out /home/asus/Descargas/cores --cpp_out /home/asus/Descargas/cores -I /home/asus/Descargas/cores --plugin=protoc-gen-grpc="/home/asus/.local/bin/grpc_cpp_plugin" /home/asus/Descargas/cores/core.proto

core.pb.h: core.pb.cc
	@$(CMAKE_COMMAND) -E touch_nocreate core.pb.h

core.grpc.pb.cc: core.pb.cc
	@$(CMAKE_COMMAND) -E touch_nocreate core.grpc.pb.cc

core.grpc.pb.h: core.pb.cc
	@$(CMAKE_COMMAND) -E touch_nocreate core.grpc.pb.h

CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.o: CMakeFiles/core_grpc_proto.dir/flags.make
CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.o: core.grpc.pb.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.o -c /home/asus/Descargas/cores/core.grpc.pb.cc

CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asus/Descargas/cores/core.grpc.pb.cc > CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.i

CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asus/Descargas/cores/core.grpc.pb.cc -o CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.s

CMakeFiles/core_grpc_proto.dir/core.pb.cc.o: CMakeFiles/core_grpc_proto.dir/flags.make
CMakeFiles/core_grpc_proto.dir/core.pb.cc.o: core.pb.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/core_grpc_proto.dir/core.pb.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/core_grpc_proto.dir/core.pb.cc.o -c /home/asus/Descargas/cores/core.pb.cc

CMakeFiles/core_grpc_proto.dir/core.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/core_grpc_proto.dir/core.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asus/Descargas/cores/core.pb.cc > CMakeFiles/core_grpc_proto.dir/core.pb.cc.i

CMakeFiles/core_grpc_proto.dir/core.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/core_grpc_proto.dir/core.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asus/Descargas/cores/core.pb.cc -o CMakeFiles/core_grpc_proto.dir/core.pb.cc.s

# Object files for target core_grpc_proto
core_grpc_proto_OBJECTS = \
"CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.o" \
"CMakeFiles/core_grpc_proto.dir/core.pb.cc.o"

# External object files for target core_grpc_proto
core_grpc_proto_EXTERNAL_OBJECTS =

libcore_grpc_proto.a: CMakeFiles/core_grpc_proto.dir/core.grpc.pb.cc.o
libcore_grpc_proto.a: CMakeFiles/core_grpc_proto.dir/core.pb.cc.o
libcore_grpc_proto.a: CMakeFiles/core_grpc_proto.dir/build.make
libcore_grpc_proto.a: CMakeFiles/core_grpc_proto.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX static library libcore_grpc_proto.a"
	$(CMAKE_COMMAND) -P CMakeFiles/core_grpc_proto.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/core_grpc_proto.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/core_grpc_proto.dir/build: libcore_grpc_proto.a

.PHONY : CMakeFiles/core_grpc_proto.dir/build

CMakeFiles/core_grpc_proto.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/core_grpc_proto.dir/cmake_clean.cmake
.PHONY : CMakeFiles/core_grpc_proto.dir/clean

CMakeFiles/core_grpc_proto.dir/depend: core.grpc.pb.cc
CMakeFiles/core_grpc_proto.dir/depend: core.grpc.pb.h
CMakeFiles/core_grpc_proto.dir/depend: core.pb.cc
CMakeFiles/core_grpc_proto.dir/depend: core.pb.h
	cd /home/asus/Descargas/cores && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores/CMakeFiles/core_grpc_proto.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/core_grpc_proto.dir/depend

