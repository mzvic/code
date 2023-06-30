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
        ${core_proto_hdrs}
        ${ZLIB_LIBRARIES})
target_link_libraries(core_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

# include_directories(reactor)
# link_directories(reactor)
# add_subdirectory(reactor)

add_library(reactor
        reactor/reactor.h
        reactor/publisher_server_reactor.cc
        reactor/subscriber_server_reactor.cc)

target_link_libraries(reactor
        core_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${ZLIB_LIBRARIES}
        ${_PROTOBUF_LIBPROTOBUF})

add_library(client
        client.h
        publisher_client.cc
        subscriber_client.cc)

target_link_libraries(client
        core_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${ZLIB_LIBRARIES}
        ${_PROTOBUF_LIBPROTOBUF})

# Targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS system iostreams)
# Find ZLib library
find_package(ZLIB REQUIRED)
# Find Python3
find_package(Python3 COMPONENTS Development)

foreach (_target
        APD_broker APD_fft APD_publisher APD_reg APD_reg_zero APD_reg_proc APD_plot_cvt APD_fft3 APD_reg_fft APD_reg_fft_avg)
    add_executable(${_target} "${_target}.cc")
    target_link_libraries(${_target}
            core_grpc_proto
            reactor
            client
            fftw3
            Python3::Python
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF}
            ${Boost_LIBRARIES} # Link Boost libraries
            ${ZLIB_LIBRARIES}
            ${_FFTW3}            
            )
endforeach ()
