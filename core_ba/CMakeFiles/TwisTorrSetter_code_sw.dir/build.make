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
CMAKE_SOURCE_DIR = /home/code/Development1/core_ba

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/code/Development1/core_ba

# Include any dependencies generated for this target.
include CMakeFiles/TwisTorrSetter_code_sw.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/TwisTorrSetter_code_sw.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/TwisTorrSetter_code_sw.dir/flags.make

CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.o: CMakeFiles/TwisTorrSetter_code_sw.dir/flags.make
CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.o: TwisTorrSetter_code_sw.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/code/Development1/core_ba/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.o -c /home/code/Development1/core_ba/TwisTorrSetter_code_sw.cc

CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/code/Development1/core_ba/TwisTorrSetter_code_sw.cc > CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.i

CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/code/Development1/core_ba/TwisTorrSetter_code_sw.cc -o CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.s

# Object files for target TwisTorrSetter_code_sw
TwisTorrSetter_code_sw_OBJECTS = \
"CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.o"

# External object files for target TwisTorrSetter_code_sw
TwisTorrSetter_code_sw_EXTERNAL_OBJECTS =

bin/TwisTorrSetter_code_sw: CMakeFiles/TwisTorrSetter_code_sw.dir/TwisTorrSetter_code_sw.cc.o
bin/TwisTorrSetter_code_sw: CMakeFiles/TwisTorrSetter_code_sw.dir/build.make
bin/TwisTorrSetter_code_sw: libcore_grpc_proto.a
bin/TwisTorrSetter_code_sw: libbroker_client.a
bin/TwisTorrSetter_code_sw: libstorage_client.a
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libcrypto.so
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libcurl.so
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libpthread.a
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libsz.so
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libz.so
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libdl.a
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libm.so
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libgrpc++_reflection.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libgrpc++.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libprotobuf.a
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libboost_system.so.1.74.0
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libboost_iostreams.so.1.74.0
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libz.so
bin/TwisTorrSetter_code_sw: libcore_grpc_proto.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libgrpc++_reflection.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libgrpc++.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libgrpc.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libcares.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libaddress_sorting.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libre2.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libupb.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libgpr.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_distributions.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_seed_sequences.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_pool_urbg.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_randen.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_randen_hwaes.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_randen_hwaes_impl.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_randen_slow.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_platform.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_internal_seed_material.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_random_seed_gen_exception.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libssl.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libcrypto.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libprotobuf.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_check_op.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_leak_check.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_die_if_null.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_conditions.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_message.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_nullguard.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_examine_stack.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_format.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_proto.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_log_sink_set.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_sink.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_entry.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_marshalling.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_reflection.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_config.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_program_name.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_private_handle_accessor.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_commandlineflag.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_flags_commandlineflag_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_initialize.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_globals.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_internal_globals.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_hash.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_city.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_low_level_hash.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_raw_hash_set.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_hashtablez_sampler.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_statusor.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_status.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_cord.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_cordz_info.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_cord_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_cordz_functions.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_exponential_biased.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_cordz_handle.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_crc_cord_state.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_crc32c.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_crc_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_crc_cpu_detect.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_bad_optional_access.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_str_format_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_strerror.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_synchronization.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_stacktrace.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_symbolize.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_debugging_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_demangle_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_graphcycles_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_malloc_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_time.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_civil_time.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_time_zone.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_bad_variant_access.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libutf8_validity.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_strings.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_throw_delegate.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_int128.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_strings_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_base.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_raw_logging_internal.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_log_severity.a
bin/TwisTorrSetter_code_sw: /home/code/.local/lib/libabsl_spinlock_wait.a
bin/TwisTorrSetter_code_sw: /usr/lib/x86_64-linux-gnu/libz.so
bin/TwisTorrSetter_code_sw: CMakeFiles/TwisTorrSetter_code_sw.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/code/Development1/core_ba/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable bin/TwisTorrSetter_code_sw"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/TwisTorrSetter_code_sw.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/TwisTorrSetter_code_sw.dir/build: bin/TwisTorrSetter_code_sw

.PHONY : CMakeFiles/TwisTorrSetter_code_sw.dir/build

CMakeFiles/TwisTorrSetter_code_sw.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/TwisTorrSetter_code_sw.dir/cmake_clean.cmake
.PHONY : CMakeFiles/TwisTorrSetter_code_sw.dir/clean

CMakeFiles/TwisTorrSetter_code_sw.dir/depend:
	cd /home/code/Development1/core_ba && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/code/Development1/core_ba /home/code/Development1/core_ba /home/code/Development1/core_ba /home/code/Development1/core_ba /home/code/Development1/core_ba/CMakeFiles/TwisTorrSetter_code_sw.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/TwisTorrSetter_code_sw.dir/depend

