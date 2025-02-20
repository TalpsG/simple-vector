cmake_minimum_required(VERSION 3.16)
project(simple_vector)
set(CMAKE_CXX_STANDARD 20)
file(GLOB SRC "./src/*.cc")

include_directories(./include)


find_package(fmt REQUIRED)
find_package(faiss REQUIRED)
find_package(OpenMP REQUIRED)
find_package(spdlog REQUIRED)
find_package(httplib REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(roaring REQUIRED)
find_path(ROCKSDB_INCLUDE_DIR rocksdb/db.h PATHS)

include_directories(${ROCKSDB_INCLUDE_DIR})
find_library(ROCKSDB_LIB rocksdb)

add_executable(simple_vector ${SRC})
target_link_libraries(simple_vector fmt::fmt ${ROCKSDB_LIB} faiss
                      spdlog roaring::roaring httplib::httplib)

