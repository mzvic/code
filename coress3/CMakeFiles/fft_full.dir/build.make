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
include CMakeFiles/fft_full.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/fft_full.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/fft_full.dir/flags.make

CMakeFiles/fft_full.dir/fft_full.cc.o: CMakeFiles/fft_full.dir/flags.make
CMakeFiles/fft_full.dir/fft_full.cc.o: fft_full.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/code/Development/coress3/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/fft_full.dir/fft_full.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fft_full.dir/fft_full.cc.o -c /home/code/Development/coress3/fft_full.cc

CMakeFiles/fft_full.dir/fft_full.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fft_full.dir/fft_full.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/code/Development/coress3/fft_full.cc > CMakeFiles/fft_full.dir/fft_full.cc.i

CMakeFiles/fft_full.dir/fft_full.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fft_full.dir/fft_full.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/code/Development/coress3/fft_full.cc -o CMakeFiles/fft_full.dir/fft_full.cc.s

# Object files for target fft_full
fft_full_OBJECTS = \
"CMakeFiles/fft_full.dir/fft_full.cc.o"

# External object files for target fft_full
fft_full_EXTERNAL_OBJECTS =

bin/fft_full: CMakeFiles/fft_full.dir/fft_full.cc.o
bin/fft_full: CMakeFiles/fft_full.dir/build.make
bin/fft_full: libcore_grpc_proto.a
bin/fft_full: libbroker_client.a
bin/fft_full: libstorage_client.a
bin/fft_full: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so
bin/fft_full: /usr/lib/x86_64-linux-gnu/libcrypto.so
bin/fft_full: /usr/lib/x86_64-linux-gnu/libcurl.so
bin/fft_full: /usr/lib/x86_64-linux-gnu/libpthread.a
bin/fft_full: /usr/lib/x86_64-linux-gnu/libsz.so
bin/fft_full: /usr/lib/x86_64-linux-gnu/libz.so
bin/fft_full: /usr/lib/x86_64-linux-gnu/libdl.a
bin/fft_full: /usr/lib/x86_64-linux-gnu/libm.so
bin/fft_full: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so
bin/fft_full: /home/code/.local/lib/libgrpc++_reflection.a
bin/fft_full: /usr/lib/x86_64-linux-gnu/libz.so
bin/fft_full: /home/code/.local/lib/libgrpc++.a
bin/fft_full: /home/code/.local/lib/libprotobuf.a
bin/fft_full: libcore_grpc_proto.a
bin/fft_full: /home/code/.local/lib/libgrpc++_reflection.a
bin/fft_full: /home/code/.local/lib/libgrpc++.a
bin/fft_full: /home/code/.local/lib/libgrpc.a
bin/fft_full: /home/code/.local/lib/libcares.a
bin/fft_full: /home/code/.local/lib/libaddress_sorting.a
bin/fft_full: /home/code/.local/lib/libre2.a
bin/fft_full: /home/code/.local/lib/libupb.a
bin/fft_full: /home/code/.local/lib/libgpr.a
bin/fft_full: /home/code/.local/lib/libabsl_random_distributions.a
bin/fft_full: /home/code/.local/lib/libabsl_random_seed_sequences.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_pool_urbg.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_randen.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_randen_hwaes.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_randen_hwaes_impl.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_randen_slow.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_platform.a
bin/fft_full: /home/code/.local/lib/libabsl_random_internal_seed_material.a
bin/fft_full: /home/code/.local/lib/libabsl_random_seed_gen_exception.a
bin/fft_full: /home/code/.local/lib/libssl.a
bin/fft_full: /home/code/.local/lib/libcrypto.a
bin/fft_full: /home/code/.local/lib/libprotobuf.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_check_op.a
bin/fft_full: /home/code/.local/lib/libabsl_leak_check.a
bin/fft_full: /home/code/.local/lib/libabsl_die_if_null.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_conditions.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_message.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_nullguard.a
bin/fft_full: /home/code/.local/lib/libabsl_examine_stack.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_format.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_proto.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_log_sink_set.a
bin/fft_full: /home/code/.local/lib/libabsl_log_sink.a
bin/fft_full: /home/code/.local/lib/libabsl_log_entry.a
bin/fft_full: /home/code/.local/lib/libabsl_flags.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_marshalling.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_reflection.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_config.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_program_name.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_private_handle_accessor.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_commandlineflag.a
bin/fft_full: /home/code/.local/lib/libabsl_flags_commandlineflag_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_log_initialize.a
bin/fft_full: /home/code/.local/lib/libabsl_log_globals.a
bin/fft_full: /home/code/.local/lib/libabsl_log_internal_globals.a
bin/fft_full: /home/code/.local/lib/libabsl_hash.a
bin/fft_full: /home/code/.local/lib/libabsl_city.a
bin/fft_full: /home/code/.local/lib/libabsl_low_level_hash.a
bin/fft_full: /home/code/.local/lib/libabsl_raw_hash_set.a
bin/fft_full: /home/code/.local/lib/libabsl_hashtablez_sampler.a
bin/fft_full: /home/code/.local/lib/libabsl_statusor.a
bin/fft_full: /home/code/.local/lib/libabsl_status.a
bin/fft_full: /home/code/.local/lib/libabsl_cord.a
bin/fft_full: /home/code/.local/lib/libabsl_cordz_info.a
bin/fft_full: /home/code/.local/lib/libabsl_cord_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_cordz_functions.a
bin/fft_full: /home/code/.local/lib/libabsl_exponential_biased.a
bin/fft_full: /home/code/.local/lib/libabsl_cordz_handle.a
bin/fft_full: /home/code/.local/lib/libabsl_crc_cord_state.a
bin/fft_full: /home/code/.local/lib/libabsl_crc32c.a
bin/fft_full: /home/code/.local/lib/libabsl_crc_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_crc_cpu_detect.a
bin/fft_full: /home/code/.local/lib/libabsl_bad_optional_access.a
bin/fft_full: /home/code/.local/lib/libabsl_str_format_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_strerror.a
bin/fft_full: /home/code/.local/lib/libabsl_synchronization.a
bin/fft_full: /home/code/.local/lib/libabsl_stacktrace.a
bin/fft_full: /home/code/.local/lib/libabsl_symbolize.a
bin/fft_full: /home/code/.local/lib/libabsl_debugging_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_demangle_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_graphcycles_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_malloc_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_time.a
bin/fft_full: /home/code/.local/lib/libabsl_civil_time.a
bin/fft_full: /home/code/.local/lib/libabsl_time_zone.a
bin/fft_full: /home/code/.local/lib/libabsl_bad_variant_access.a
bin/fft_full: /home/code/.local/lib/libutf8_validity.a
bin/fft_full: /home/code/.local/lib/libabsl_strings.a
bin/fft_full: /home/code/.local/lib/libabsl_throw_delegate.a
bin/fft_full: /home/code/.local/lib/libabsl_int128.a
bin/fft_full: /home/code/.local/lib/libabsl_strings_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_base.a
bin/fft_full: /home/code/.local/lib/libabsl_raw_logging_internal.a
bin/fft_full: /home/code/.local/lib/libabsl_log_severity.a
bin/fft_full: /home/code/.local/lib/libabsl_spinlock_wait.a
bin/fft_full: /usr/lib/x86_64-linux-gnu/libz.so
bin/fft_full: CMakeFiles/fft_full.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/code/Development/coress3/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/fft_full"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fft_full.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/fft_full.dir/build: bin/fft_full

.PHONY : CMakeFiles/fft_full.dir/build

CMakeFiles/fft_full.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/fft_full.dir/cmake_clean.cmake
.PHONY : CMakeFiles/fft_full.dir/clean

CMakeFiles/fft_full.dir/depend:
	cd /home/code/Development/coress3 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/code/Development/coress3 /home/code/Development/coress3 /home/code/Development/coress3 /home/code/Development/coress3 /home/code/Development/coress3/CMakeFiles/fft_full.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/fft_full.dir/depend

