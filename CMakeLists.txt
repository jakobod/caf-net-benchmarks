# -- project setup -------------------------------------------------------------

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(caf_net_benchmarks CXX)

SET(CMAKE_C_FLAGS_DEBUG "-D_DEBUG")

# Set module path appropriately.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# -- set useful CMake options --------------------------------------------------

add_compile_options("-std=c++17")

# Enable ASAN if requested by the user.
if (CAF_ENABLE_ADDRESS_SANITIZER AND NOT WIN32)
  add_compile_options("-fsanitize=address"
                      "-fno-omit-frame-pointer")
  list(APPEND CAF_EXTRA_LDFLAGS "-fsanitize=address")
endif ()

# -pthread is ignored on MacOSX but required on other platforms
if (NOT APPLE AND NOT WIN32)
  add_compile_options(-pthread)
  list(APPEND CAF_EXTRA_LDFLAGS "-pthread")
endif ()

# -- find CAF and CAF_NET ------------------------------------------------------

find_package(CAF COMPONENTS core io REQUIRED)
find_package(CAF_NET REQUIRED)

include_directories(header
                    "${CMAKE_BINARY_DIR}"
                    "${CAF_INCLUDE_DIRS}"
                    "${CAF_NET_INCLUDE_DIRS}")

message(STATUS "CAF_NET_INCLUDE_DIRS: ${CAF_NET_INCLUDE_DIRS}")

# -- add targets ---------------------------------------------------------------

# Utility macro for adding benchmark targets
macro(add_target name)
  add_executable(${name}
                  "src/${name}.cpp"
                  src/utility.cpp)
  target_link_libraries(${name}
                        ${CAF_EXTRA_LDFLAGS}
                        ${CAF_LIBRARIES}
                        ${CAF_NET_LIBRARIES}
                        ${PTHREAD_LIBRARIES})
endmacro()

add_target(caf_streaming_tcp)
add_target(pingpong_tcp)
add_target(pingpong_udp)
add_target(streaming_tcp)
