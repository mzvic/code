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
CMAKE_COMMAND = /home/code/.local/bin/cmake

# The command to remove a file.
RM = /home/code/.local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/code/Development/core

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/code/Development/core

# Include any dependencies generated for this target.
include CMakeFiles/TwisTorrSS1_code_sw.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/TwisTorrSS1_code_sw.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/TwisTorrSS1_code_sw.dir/flags.make

CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.o: CMakeFiles/TwisTorrSS1_code_sw.dir/flags.make
CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.o: TwisTorrSS1_code_sw.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/code/Development/core/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.o -c /home/code/Development/core/TwisTorrSS1_code_sw.cc

CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/code/Development/core/TwisTorrSS1_code_sw.cc > CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.i

CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/code/Development/core/TwisTorrSS1_code_sw.cc -o CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.s

# Object files for target TwisTorrSS1_code_sw
TwisTorrSS1_code_sw_OBJECTS = \
"CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.o"

# External object files for target TwisTorrSS1_code_sw
TwisTorrSS1_code_sw_EXTERNAL_OBJECTS =

bin/TwisTorrSS1_code_sw: CMakeFiles/TwisTorrSS1_code_sw.dir/TwisTorrSS1_code_sw.cc.o
bin/TwisTorrSS1_code_sw: CMakeFiles/TwisTorrSS1_code_sw.dir/build.make
bin/TwisTorrSS1_code_sw: libcore_grpc_proto.a
bin/TwisTorrSS1_code_sw: libbroker_client.a
bin/TwisTorrSS1_code_sw: libstorage_client.a
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libcrypto.so
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libcurl.so
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libpthread.a
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libsz.so
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libz.so
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libdl.a
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libm.so
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libgrpc++_reflection.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libgrpc++.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libprotobuf.a
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libboost_system.so.1.74.0
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libboost_iostreams.so.1.74.0
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libz.so
bin/TwisTorrSS1_code_sw: libcore_grpc_proto.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libgrpc++_reflection.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libgrpc++.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libgrpc.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libcares.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libaddress_sorting.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libre2.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libupb.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libgpr.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_distributions.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_seed_sequences.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_pool_urbg.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_randen.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_randen_hwaes.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_randen_hwaes_impl.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_randen_slow.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_platform.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_internal_seed_material.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_random_seed_gen_exception.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libssl.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libcrypto.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libprotobuf.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_check_op.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_leak_check.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_die_if_null.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_conditions.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_message.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_nullguard.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_examine_stack.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_format.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_proto.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_log_sink_set.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_sink.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_entry.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_marshalling.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_reflection.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_config.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_program_name.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_private_handle_accessor.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_commandlineflag.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_flags_commandlineflag_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_initialize.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_globals.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_internal_globals.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_hash.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_city.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_low_level_hash.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_raw_hash_set.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_hashtablez_sampler.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_statusor.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_status.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_cord.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_cordz_info.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_cord_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_cordz_functions.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_exponential_biased.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_cordz_handle.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_crc_cord_state.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_crc32c.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_crc_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_crc_cpu_detect.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_bad_optional_access.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_str_format_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_strerror.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_synchronization.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_stacktrace.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_symbolize.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_debugging_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_demangle_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_graphcycles_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_malloc_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_time.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_civil_time.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_time_zone.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_bad_variant_access.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libutf8_validity.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_strings.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_throw_delegate.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_int128.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_strings_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_base.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_raw_logging_internal.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_log_severity.a
bin/TwisTorrSS1_code_sw: /home/code/.local/lib/libabsl_spinlock_wait.a
bin/TwisTorrSS1_code_sw: /usr/lib/x86_64-linux-gnu/libz.so
bin/TwisTorrSS1_code_sw: CMakeFiles/TwisTorrSS1_code_sw.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/code/Development/core/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/TwisTorrSS1_code_sw"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/TwisTorrSS1_code_sw.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/TwisTorrSS1_code_sw.dir/build: bin/TwisTorrSS1_code_sw

.PHONY : CMakeFiles/TwisTorrSS1_code_sw.dir/build

CMakeFiles/TwisTorrSS1_code_sw.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/TwisTorrSS1_code_sw.dir/cmake_clean.cmake
.PHONY : CMakeFiles/TwisTorrSS1_code_sw.dir/clean

CMakeFiles/TwisTorrSS1_code_sw.dir/depend:
	cd /home/code/Development/core && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/code/Development/core /home/code/Development/core /home/code/Development/core /home/code/Development/core /home/code/Development/core/CMakeFiles/TwisTorrSS1_code_sw.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/TwisTorrSS1_code_sw.dir/depend

