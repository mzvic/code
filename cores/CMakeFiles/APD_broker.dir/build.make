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
include CMakeFiles/APD_broker.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/APD_broker.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/APD_broker.dir/flags.make

CMakeFiles/APD_broker.dir/APD_broker.cc.o: CMakeFiles/APD_broker.dir/flags.make
CMakeFiles/APD_broker.dir/APD_broker.cc.o: APD_broker.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/APD_broker.dir/APD_broker.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/APD_broker.dir/APD_broker.cc.o -c /home/asus/Descargas/cores/APD_broker.cc

CMakeFiles/APD_broker.dir/APD_broker.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/APD_broker.dir/APD_broker.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asus/Descargas/cores/APD_broker.cc > CMakeFiles/APD_broker.dir/APD_broker.cc.i

CMakeFiles/APD_broker.dir/APD_broker.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/APD_broker.dir/APD_broker.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asus/Descargas/cores/APD_broker.cc -o CMakeFiles/APD_broker.dir/APD_broker.cc.s

# Object files for target APD_broker
APD_broker_OBJECTS = \
"CMakeFiles/APD_broker.dir/APD_broker.cc.o"

# External object files for target APD_broker
APD_broker_EXTERNAL_OBJECTS =

bin/APD_broker: CMakeFiles/APD_broker.dir/APD_broker.cc.o
bin/APD_broker: CMakeFiles/APD_broker.dir/build.make
bin/APD_broker: libcore_grpc_proto.a
bin/APD_broker: libreactor.a
bin/APD_broker: libclient.a
bin/APD_broker: /usr/lib/x86_64-linux-gnu/libpython3.10.so
bin/APD_broker: /home/asus/.local/lib/libgrpc++_reflection.a
bin/APD_broker: /home/asus/.local/lib/libgrpc++.a
bin/APD_broker: /home/asus/.local/lib/libprotobuf.a
bin/APD_broker: /usr/lib/x86_64-linux-gnu/libboost_system.so.1.74.0
bin/APD_broker: /usr/lib/x86_64-linux-gnu/libboost_iostreams.so.1.74.0
bin/APD_broker: /home/asus/.local/lib/libz.so
bin/APD_broker: libcore_grpc_proto.a
bin/APD_broker: /home/asus/.local/lib/libgrpc++_reflection.a
bin/APD_broker: /home/asus/.local/lib/libgrpc++.a
bin/APD_broker: /home/asus/.local/lib/libgrpc.a
bin/APD_broker: /home/asus/.local/lib/libz.a
bin/APD_broker: /home/asus/.local/lib/libcares.a
bin/APD_broker: /home/asus/.local/lib/libaddress_sorting.a
bin/APD_broker: /home/asus/.local/lib/libre2.a
bin/APD_broker: /home/asus/.local/lib/libupb.a
bin/APD_broker: /home/asus/.local/lib/libabsl_raw_hash_set.a
bin/APD_broker: /home/asus/.local/lib/libabsl_hashtablez_sampler.a
bin/APD_broker: /home/asus/.local/lib/libabsl_hash.a
bin/APD_broker: /home/asus/.local/lib/libabsl_city.a
bin/APD_broker: /home/asus/.local/lib/libabsl_low_level_hash.a
bin/APD_broker: /home/asus/.local/lib/libabsl_statusor.a
bin/APD_broker: /home/asus/.local/lib/libgpr.a
bin/APD_broker: /home/asus/.local/lib/libabsl_bad_variant_access.a
bin/APD_broker: /home/asus/.local/lib/libabsl_status.a
bin/APD_broker: /home/asus/.local/lib/libabsl_strerror.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_distributions.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_seed_sequences.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_pool_urbg.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_randen.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_randen_hwaes.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_randen_hwaes_impl.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_randen_slow.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_platform.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_internal_seed_material.a
bin/APD_broker: /home/asus/.local/lib/libabsl_random_seed_gen_exception.a
bin/APD_broker: /home/asus/.local/lib/libabsl_cord.a
bin/APD_broker: /home/asus/.local/lib/libabsl_bad_optional_access.a
bin/APD_broker: /home/asus/.local/lib/libabsl_cordz_info.a
bin/APD_broker: /home/asus/.local/lib/libabsl_cord_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_cordz_functions.a
bin/APD_broker: /home/asus/.local/lib/libabsl_exponential_biased.a
bin/APD_broker: /home/asus/.local/lib/libabsl_cordz_handle.a
bin/APD_broker: /home/asus/.local/lib/libabsl_str_format_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_synchronization.a
bin/APD_broker: /home/asus/.local/lib/libabsl_stacktrace.a
bin/APD_broker: /home/asus/.local/lib/libabsl_symbolize.a
bin/APD_broker: /home/asus/.local/lib/libabsl_debugging_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_demangle_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_graphcycles_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_malloc_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_time.a
bin/APD_broker: /home/asus/.local/lib/libabsl_strings.a
bin/APD_broker: /home/asus/.local/lib/libabsl_throw_delegate.a
bin/APD_broker: /home/asus/.local/lib/libabsl_int128.a
bin/APD_broker: /home/asus/.local/lib/libabsl_strings_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_base.a
bin/APD_broker: /home/asus/.local/lib/libabsl_spinlock_wait.a
bin/APD_broker: /home/asus/.local/lib/libabsl_raw_logging_internal.a
bin/APD_broker: /home/asus/.local/lib/libabsl_log_severity.a
bin/APD_broker: /home/asus/.local/lib/libabsl_civil_time.a
bin/APD_broker: /home/asus/.local/lib/libabsl_time_zone.a
bin/APD_broker: /home/asus/.local/lib/libssl.a
bin/APD_broker: /home/asus/.local/lib/libcrypto.a
bin/APD_broker: /home/asus/.local/lib/libprotobuf.a
bin/APD_broker: CMakeFiles/APD_broker.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/asus/Descargas/cores/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/APD_broker"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/APD_broker.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/APD_broker.dir/build: bin/APD_broker

.PHONY : CMakeFiles/APD_broker.dir/build

CMakeFiles/APD_broker.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/APD_broker.dir/cmake_clean.cmake
.PHONY : CMakeFiles/APD_broker.dir/clean

CMakeFiles/APD_broker.dir/depend:
	cd /home/asus/Descargas/cores && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores /home/asus/Descargas/cores/CMakeFiles/APD_broker.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/APD_broker.dir/depend

