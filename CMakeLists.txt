cmake_minimum_required(VERSION 3.16)
project(simple_vector)
set(CMAKE_CXX_STANDARD 20)
set(
  SRC
  ./src/main.cc
  ./src/FaissIndex.cc
  ./src/logger.cc
)

include_directories(./include)


find_package(fmt REQUIRED)
find_package(faiss REQUIRED)
find_package(spdlog REQUIRED)
find_package(httplib REQUIRED)
find_package(RapidJSON REQUIRED)

add_executable(simple_vector ${SRC})
target_link_libraries(simple_vector fmt::fmt)

