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
CMAKE_SOURCE_DIR = /home/code/Development/coress3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/code/Development/coress3

# Include any dependencies generated for this target.
include CMakeFiles/recorder.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/recorder.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/recorder.dir/flags.make

CMakeFiles/recorder.dir/recorder.cc.o: CMakeFiles/recorder.dir/flags.make
CMakeFiles/recorder.dir/recorder.cc.o: recorder.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/code/Development/coress3/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/recorder.dir/recorder.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/recorder.dir/recorder.cc.o -c /home/code/Development/coress3/recorder.cc

CMakeFiles/recorder.dir/recorder.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/recorder.dir/recorder.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/code/Development/coress3/recorder.cc > CMakeFiles/recorder.dir/recorder.cc.i

CMakeFiles/recorder.dir/recorder.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/recorder.dir/recorder.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/code/Development/coress3/recorder.cc -o CMakeFiles/recorder.dir/recorder.cc.s

# Object files for target recorder
recorder_OBJECTS = \
"CMakeFiles/recorder.dir/recorder.cc.o"

# External object files for target recorder
recorder_EXTERNAL_OBJECTS =

bin/recorder: CMakeFiles/recorder.dir/recorder.cc.o
bin/recorder: CMakeFiles/recorder.dir/build.make
bin/recorder: libcore_grpc_proto.a
bin/recorder: libbroker_client.a
bin/recorder: libstorage_client.a
bin/recorder: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so
bin/recorder: /usr/lib/x86_64-linux-gnu/libcrypto.so
bin/recorder: /usr/lib/x86_64-linux-gnu/libcurl.so
bin/recorder: /usr/lib/x86_64-linux-gnu/libpthread.a
bin/recorder: /usr/lib/x86_64-linux-gnu/libsz.so
bin/recorder: /usr/lib/x86_64-linux-gnu/libz.so
bin/recorder: /usr/lib/x86_64-linux-gnu/libdl.a
bin/recorder: /usr/lib/x86_64-linux-gnu/libm.so
bin/recorder: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so
bin/recorder: /home/code/.local/lib/libgrpc++_reflection.a
bin/recorder: /usr/lib/x86_64-linux-gnu/libz.so
bin/recorder: /home/code/.local/lib/libgrpc++.a
bin/recorder: /home/code/.local/lib/libprotobuf.a
bin/recorder: libcore_grpc_proto.a
bin/recorder: /home/code/.local/lib/libgrpc++_reflection.a
bin/recorder: /home/code/.local/lib/libgrpc++.a
bin/recorder: /home/code/.local/lib/libgrpc.a
bin/recorder: /home/code/.local/lib/libcares.a
bin/recorder: /home/code/.local/lib/libaddress_sorting.a
bin/recorder: /home/code/.local/lib/libre2.a
bin/recorder: /home/code/.local/lib/libupb.a
bin/recorder: /home/code/.local/lib/libgpr.a
bin/recorder: /home/code/.local/lib/libabsl_random_distributions.a
bin/recorder: /home/code/.local/lib/libabsl_random_seed_sequences.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_pool_urbg.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_randen.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_randen_hwaes.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_randen_hwaes_impl.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_randen_slow.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_platform.a
bin/recorder: /home/code/.local/lib/libabsl_random_internal_seed_material.a
bin/recorder: /home/code/.local/lib/libabsl_random_seed_gen_exception.a
bin/recorder: /home/code/.local/lib/libssl.a
bin/recorder: /home/code/.local/lib/libcrypto.a
bin/recorder: /home/code/.local/lib/libprotobuf.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_check_op.a
bin/recorder: /home/code/.local/lib/libabsl_leak_check.a
bin/recorder: /home/code/.local/lib/libabsl_die_if_null.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_conditions.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_message.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_nullguard.a
bin/recorder: /home/code/.local/lib/libabsl_examine_stack.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_format.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_proto.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_log_sink_set.a
bin/recorder: /home/code/.local/lib/libabsl_log_sink.a
bin/recorder: /home/code/.local/lib/libabsl_log_entry.a
bin/recorder: /home/code/.local/lib/libabsl_flags.a
bin/recorder: /home/code/.local/lib/libabsl_flags_internal.a
bin/recorder: /home/code/.local/lib/libabsl_flags_marshalling.a
bin/recorder: /home/code/.local/lib/libabsl_flags_reflection.a
bin/recorder: /home/code/.local/lib/libabsl_flags_config.a
bin/recorder: /home/code/.local/lib/libabsl_flags_program_name.a
bin/recorder: /home/code/.local/lib/libabsl_flags_private_handle_accessor.a
bin/recorder: /home/code/.local/lib/libabsl_flags_commandlineflag.a
bin/recorder: /home/code/.local/lib/libabsl_flags_commandlineflag_internal.a
bin/recorder: /home/code/.local/lib/libabsl_log_initialize.a
bin/recorder: /home/code/.local/lib/libabsl_log_globals.a
bin/recorder: /home/code/.local/lib/libabsl_log_internal_globals.a
bin/recorder: /home/code/.local/lib/libabsl_hash.a
bin/recorder: /home/code/.local/lib/libabsl_city.a
bin/recorder: /home/code/.local/lib/libabsl_low_level_hash.a
bin/recorder: /home/code/.local/lib/libabsl_raw_hash_set.a
bin/recorder: /home/code/.local/lib/libabsl_hashtablez_sampler.a
bin/recorder: /home/code/.local/lib/libabsl_statusor.a
bin/recorder: /home/code/.local/lib/libabsl_status.a
bin/recorder: /home/code/.local/lib/libabsl_cord.a
bin/recorder: /home/code/.local/lib/libabsl_cordz_info.a
bin/recorder: /home/code/.local/lib/libabsl_cord_internal.a
bin/recorder: /home/code/.local/lib/libabsl_cordz_functions.a
bin/recorder: /home/code/.local/lib/libabsl_exponential_biased.a
bin/recorder: /home/code/.local/lib/libabsl_cordz_handle.a
bin/recorder: /home/code/.local/lib/libabsl_crc_cord_state.a
bin/recorder: /home/code/.local/lib/libabsl_crc32c.a
bin/recorder: /home/code/.local/lib/libabsl_crc_internal.a
bin/recorder: /home/code/.local/lib/libabsl_crc_cpu_detect.a
bin/recorder: /home/code/.local/lib/libabsl_bad_optional_access.a
bin/recorder: /home/code/.local/lib/libabsl_str_format_internal.a
bin/recorder: /home/code/.local/lib/libabsl_strerror.a
bin/recorder: /home/code/.local/lib/libabsl_synchronization.a
bin/recorder: /home/code/.local/lib/libabsl_stacktrace.a
bin/recorder: /home/code/.local/lib/libabsl_symbolize.a
bin/recorder: /home/code/.local/lib/libabsl_debugging_internal.a
bin/recorder: /home/code/.local/lib/libabsl_demangle_internal.a
bin/recorder: /home/code/.local/lib/libabsl_graphcycles_internal.a
bin/recorder: /home/code/.local/lib/libabsl_malloc_internal.a
bin/recorder: /home/code/.local/lib/libabsl_time.a
bin/recorder: /home/code/.local/lib/libabsl_civil_time.a
bin/recorder: /home/code/.local/lib/libabsl_time_zone.a
bin/recorder: /home/code/.local/lib/libabsl_bad_variant_access.a
bin/recorder: /home/code/.local/lib/libutf8_validity.a
bin/recorder: /home/code/.local/lib/libabsl_strings.a
bin/recorder: /home/code/.local/lib/libabsl_throw_delegate.a
bin/recorder: /home/code/.local/lib/libabsl_int128.a
bin/recorder: /home/code/.local/lib/libabsl_strings_internal.a
bin/recorder: /home/code/.local/lib/libabsl_base.a
bin/recorder: /home/code/.local/lib/libabsl_raw_logging_internal.a
bin/recorder: /home/code/.local/lib/libabsl_log_severity.a
bin/recorder: /home/code/.local/lib/libabsl_spinlock_wait.a
bin/recorder: /usr/lib/x86_64-linux-gnu/libz.so
bin/recorder: CMakeFiles/recorder.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/code/Development/coress3/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/recorder"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/recorder.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/recorder.dir/build: bin/recorder

.PHONY : CMakeFiles/recorder.dir/build

CMakeFiles/recorder.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/recorder.dir/cmake_clean.cmake
.PHONY : CMakeFiles/recorder.dir/clean

CMakeFiles/recorder.dir/depend:
	cd /home/code/Development/coress3 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/code/Development/coress3 /home/code/Development/coress3 /home/code/Development/coress3 /home/code/Development/coress3 /home/code/Development/coress3/CMakeFiles/recorder.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/recorder.dir/depend
