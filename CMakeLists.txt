#----------------------------------------------------------------------
#----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.1)

project(grpc-perf C CXX)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  cmake_policy(SET CMP0091 NEW)
  add_definitions(-D_WIN32_WINNT=0x600 -bigobj)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Threads REQUIRED)

#----------------------------------------------------------------------
# Include the gRPC's cmake build
#----------------------------------------------------------------------
add_subdirectory(third_party/grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)

#----------------------------------------------------------------------
# Use the grpc targets directly from this build.
#----------------------------------------------------------------------
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

#----------------------------------------------------------------------
# Proto file
#----------------------------------------------------------------------
get_filename_component(perftest_proto "perftest.proto" ABSOLUTE)
get_filename_component(perftest_proto_path "${perftest_proto}" PATH)

#----------------------------------------------------------------------
# Generated sources
#----------------------------------------------------------------------
set(perftest_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/perftest.pb.cc")
set(perftest_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/perftest.pb.h")
set(perftest_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/perftest.grpc.pb.cc")
set(perftest_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/perftest.grpc.pb.h")
add_custom_command(
  OUTPUT "${perftest_proto_srcs}" "${perftest_proto_hdrs}" "${perftest_grpc_srcs}" "${perftest_grpc_hdrs}"
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${perftest_proto_path}" -I "${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${perftest_proto}"
  DEPENDS "${perftest_proto}")

#----------------------------------------------------------------------
# Include generated *.pb.h files
#----------------------------------------------------------------------
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "./src" "./third_party/grpc")

#----------------------------------------------------------------------
# perftest gRPC Server
#----------------------------------------------------------------------
add_executable(perftest_server
  src/perftest_server.cc
  src/client_utilities.cc
  src/performance_tests.cc
  ${perftest_proto_srcs}
  ${perftest_grpc_srcs}
  )
target_link_libraries(perftest_server
   ${_REFLECTION}
   ${_GRPC_GRPCPP}
   ${_PROTOBUF_LIBPROTOBUF}
   )

#----------------------------------------------------------------------
# Example C++ application to talk to the perftest gRPC service
#----------------------------------------------------------------------
add_executable(perftest_client
  src/perftest_client.cc
  src/client_utilities.cc
  src/performance_tests.cc
  ${perftest_proto_srcs}
  ${perftest_grpc_srcs}
  )
target_link_libraries(perftest_client
   ${_REFLECTION}
   ${_GRPC_GRPCPP}
   ${_PROTOBUF_LIBPROTOBUF})
