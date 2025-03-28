cmake_minimum_required(VERSION 3.5.1)

project(core C CXX)

include(common.cmake)

# Proto file
get_filename_component(core_proto "core.proto" ABSOLUTE)
get_filename_component(core_proto_path "${core_proto}" PATH)

# Generated sources
set(core_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/core.pb.cc")
set(core_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/core.pb.h")
set(core_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/core.grpc.pb.cc")
set(core_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/core.grpc.pb.h")
add_custom_command(
        OUTPUT "${core_proto_srcs}" "${core_proto_hdrs}" "${core_grpc_srcs}" "${core_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${core_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${core_proto}"
        DEPENDS "${core_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# core_grpc_proto
add_library(core_grpc_proto
        ${core_grpc_srcs}
        ${core_grpc_hdrs}
        ${core_proto_srcs}
        ${core_proto_hdrs})
target_link_libraries(core_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${ZLIB_LIBRARIES}
        ${_PROTOBUF_LIBPROTOBUF})

# HDF5 stuff
find_package(HDF5 COMPONENTS C HL)
include_directories(${HDF5_INCLUDE_DIRS})

add_library(broker_client
        broker_client.h
        broker_client.cc)
target_link_libraries(broker_client
        core_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

add_library(storage_client
        storage_client.h
        storage_client.cc)
target_link_libraries(storage_client
        core_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})


# Targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS system iostreams)
# Find ZLib library
find_package(ZLIB REQUIRED)

foreach (_target
        APD_broker2_code_sw APD_plot_cvt_code_sw APD_publisher_code_sw APD_fft_partial_code_sw APD_fft_full_code_sw TwisTorrIO_code_sw TwisTorrSetter_code_sw TwisTorrMonitor_code_sw storage recorder TwisTorrSS1_code_sw TwisTorrSS2_code_sw PrevacMonitor_code_sw PrevacSetter_code_sw) 
    add_executable(${_target} "${_target}.cc")
    target_link_libraries(${_target}
            core_grpc_proto
            broker_client
            storage_client
            fftw3
            fftw3_threads
            pthread
            ${HDF5_LIBRARIES}
            ${HDF5_HL_LIBRARIES}            
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF}
            ${Boost_LIBRARIES} # Link Boost libraries
            ${ZLIB_LIBRARIES}
            ${_FFTW3}            
            )
endforeach ()
