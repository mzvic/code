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
CMAKE_SOURCE_DIR = /home/asus/Documentos/code/coress2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/asus/Documentos/code/coress2

# Include any dependencies generated for this target.
include CMakeFiles/fft4.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/fft4.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/fft4.dir/flags.make

CMakeFiles/fft4.dir/fft4.cc.o: CMakeFiles/fft4.dir/flags.make
CMakeFiles/fft4.dir/fft4.cc.o: fft4.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/asus/Documentos/code/coress2/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/fft4.dir/fft4.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fft4.dir/fft4.cc.o -c /home/asus/Documentos/code/coress2/fft4.cc

CMakeFiles/fft4.dir/fft4.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fft4.dir/fft4.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/asus/Documentos/code/coress2/fft4.cc > CMakeFiles/fft4.dir/fft4.cc.i

CMakeFiles/fft4.dir/fft4.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fft4.dir/fft4.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/asus/Documentos/code/coress2/fft4.cc -o CMakeFiles/fft4.dir/fft4.cc.s

# Object files for target fft4
fft4_OBJECTS = \
"CMakeFiles/fft4.dir/fft4.cc.o"

# External object files for target fft4
fft4_EXTERNAL_OBJECTS =

bin/fft4: CMakeFiles/fft4.dir/fft4.cc.o
bin/fft4: CMakeFiles/fft4.dir/build.make
bin/fft4: libcore_grpc_proto.a
bin/fft4: libreactor.a
bin/fft4: libclient.a
bin/fft4: /home/asus/.local/lib/libgrpc++_reflection.a
bin/fft4: /home/asus/.local/lib/libgrpc++.a
bin/fft4: /home/asus/.local/lib/libprotobuf.a
bin/fft4: libcore_grpc_proto.a
bin/fft4: /home/asus/.local/lib/libgrpc++_reflection.a
bin/fft4: /home/asus/.local/lib/libgrpc++.a
bin/fft4: /home/asus/.local/lib/libgrpc.a
bin/fft4: /home/asus/.local/lib/libz.a
bin/fft4: /home/asus/.local/lib/libcares.a
bin/fft4: /home/asus/.local/lib/libaddress_sorting.a
bin/fft4: /home/asus/.local/lib/libre2.a
bin/fft4: /home/asus/.local/lib/libupb.a
bin/fft4: /home/asus/.local/lib/libabsl_raw_hash_set.a
bin/fft4: /home/asus/.local/lib/libabsl_hashtablez_sampler.a
bin/fft4: /home/asus/.local/lib/libabsl_hash.a
bin/fft4: /home/asus/.local/lib/libabsl_city.a
bin/fft4: /home/asus/.local/lib/libabsl_low_level_hash.a
bin/fft4: /home/asus/.local/lib/libabsl_statusor.a
bin/fft4: /home/asus/.local/lib/libgpr.a
bin/fft4: /home/asus/.local/lib/libabsl_bad_variant_access.a
bin/fft4: /home/asus/.local/lib/libabsl_status.a
bin/fft4: /home/asus/.local/lib/libabsl_strerror.a
bin/fft4: /home/asus/.local/lib/libabsl_random_distributions.a
bin/fft4: /home/asus/.local/lib/libabsl_random_seed_sequences.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_pool_urbg.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_randen.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_randen_hwaes.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_randen_hwaes_impl.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_randen_slow.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_platform.a
bin/fft4: /home/asus/.local/lib/libabsl_random_internal_seed_material.a
bin/fft4: /home/asus/.local/lib/libabsl_random_seed_gen_exception.a
bin/fft4: /home/asus/.local/lib/libabsl_cord.a
bin/fft4: /home/asus/.local/lib/libabsl_bad_optional_access.a
bin/fft4: /home/asus/.local/lib/libabsl_cordz_info.a
bin/fft4: /home/asus/.local/lib/libabsl_cord_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_cordz_functions.a
bin/fft4: /home/asus/.local/lib/libabsl_exponential_biased.a
bin/fft4: /home/asus/.local/lib/libabsl_cordz_handle.a
bin/fft4: /home/asus/.local/lib/libabsl_str_format_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_synchronization.a
bin/fft4: /home/asus/.local/lib/libabsl_stacktrace.a
bin/fft4: /home/asus/.local/lib/libabsl_symbolize.a
bin/fft4: /home/asus/.local/lib/libabsl_debugging_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_demangle_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_graphcycles_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_malloc_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_time.a
bin/fft4: /home/asus/.local/lib/libabsl_strings.a
bin/fft4: /home/asus/.local/lib/libabsl_throw_delegate.a
bin/fft4: /home/asus/.local/lib/libabsl_int128.a
bin/fft4: /home/asus/.local/lib/libabsl_strings_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_base.a
bin/fft4: /home/asus/.local/lib/libabsl_spinlock_wait.a
bin/fft4: /home/asus/.local/lib/libabsl_raw_logging_internal.a
bin/fft4: /home/asus/.local/lib/libabsl_log_severity.a
bin/fft4: /home/asus/.local/lib/libabsl_civil_time.a
bin/fft4: /home/asus/.local/lib/libabsl_time_zone.a
bin/fft4: /home/asus/.local/lib/libssl.a
bin/fft4: /home/asus/.local/lib/libcrypto.a
bin/fft4: /home/asus/.local/lib/libprotobuf.a
bin/fft4: CMakeFiles/fft4.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/asus/Documentos/code/coress2/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/fft4"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fft4.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/fft4.dir/build: bin/fft4

.PHONY : CMakeFiles/fft4.dir/build

CMakeFiles/fft4.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/fft4.dir/cmake_clean.cmake
.PHONY : CMakeFiles/fft4.dir/clean

CMakeFiles/fft4.dir/depend:
	cd /home/asus/Documentos/code/coress2 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/asus/Documentos/code/coress2 /home/asus/Documentos/code/coress2 /home/asus/Documentos/code/coress2 /home/asus/Documentos/code/coress2 /home/asus/Documentos/code/coress2/CMakeFiles/fft4.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/fft4.dir/depend

