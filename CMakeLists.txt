cmake_minimum_required(VERSION 3.16)
project(simple_vector)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "~/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
file(GLOB SRC "src/*.cc")
file(GLOB INC "include/*.h")


file(GLOB MSERVER_SRC "master-server/src/*.cc")
file(GLOB MSERVER_INC "master-server/include/*.h")

include_directories(./include)
include_directories(./master-server/include)


set(Protobuf_DIR "/home/talps/.local/share/vcpkg/installed/x64-linux/share/protobuf")
find_package(Protobuf REQUIRED)
set(absl_DIR "/home/talps/.local/share/vcpkg/installed/x64-linux/share/absl")
find_package(absl REQUIRED)
set(gRPC_DIR "/home/talps/.local/share/vcpkg/installed/x64-linux/share/grpc")
find_package(gRPC CONFIG REQUIRED)
find_package(fmt REQUIRED)
find_package(faiss REQUIRED)
find_package(OpenMP REQUIRED)
find_package(spdlog REQUIRED)
find_package(httplib REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(roaring REQUIRED)
find_package(NuRaft REQUIRED)
set(cpprestsdk_DIR "/home/talps/.local/share/vcpkg/installed/x64-linux/share/cpprestsdk")
find_package(cpprestsdk CONFIG REQUIRED)
set(etcd-cpp-api_DIR "/home/talps/.local/share/vcpkg/installed/x64-linux/share/etcd-cpp-apiv3")
find_package(etcd-cpp-api CONFIG REQUIRED)
find_package(CURL REQUIRED)

find_path(ROCKSDB_INCLUDE_DIR rocksdb/db.h PATHS)

include_directories(${ROCKSDB_INCLUDE_DIR})
find_library(ROCKSDB_LIB rocksdb)


add_executable(simple_vector ${SRC})
target_link_libraries(simple_vector fmt::fmt ${ROCKSDB_LIB} faiss
  spdlog roaring::roaring httplib::httplib nuraft)

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if(NOT CLANG_FORMAT_EXECUTABLE)
  message(WARNING "clang-format is not found, please install it.")
endif()

add_custom_target(format
COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${SRC} ${INC}
COMMENT "Running clang-format on source files"
)


add_executable(master-server ${MSERVER_SRC})
target_link_libraries(master-server cpprestsdk::cpprest etcd-cpp-api httplib::httplib fmt::fmt CURL::libcurl gRPC::grpc++)