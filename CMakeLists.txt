# -- project setup -------------------------------------------------------------

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(caf_net_benchmarks C CXX)

# Set module path appropriately.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# -- utility functions ---------------------------------------------------------

# Appends `str` to the variable named `var` with a whitespace as separator.
# Suppresses a leading whitespace if the variable is empty and does nothing if
# `str` is empty.
function(build_string var str)
  if(NOT str STREQUAL "")
    if("${${var}}" STREQUAL "")
      set("${var}" "${str}" PARENT_SCOPE)
    else()
      set("${var}" "${${var}} ${str}" PARENT_SCOPE)
    endif()
  endif()
endfunction(build_string)

# -- set useful CMake options --------------------------------------------------

add_compile_options("-std=c++17")

# Enable a ton of warnings if --more-clang-warnings is used.
if(CAF_MORE_WARNINGS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(WFLAGS "-Weverything -Wno-c++98-compat -Wno-padded "
               "-Wno-documentation-unknown-command -Wno-exit-time-destructors "
               "-Wno-global-constructors -Wno-missing-prototypes "
               "-Wno-c++98-compat-pedantic -Wno-unused-member-function "
               "-Wno-unused-const-variable -Wno-switch-enum "
               "-Wno-abstract-vbase-init -Wno-shadow "
               "-Wno-missing-noreturn -Wno-covered-switch-default")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(WFLAGS "-Waddress -Wall -Warray-bounds "
               "-Wattributes -Wbuiltin-macro-redefined -Wcast-align "
               "-Wcast-qual -Wchar-subscripts -Wclobbered -Wcomment "
               "-Wconversion -Wconversion-null -Wcoverage-mismatch "
               "-Wcpp -Wdelete-non-virtual-dtor -Wdeprecated "
               "-Wdeprecated-declarations -Wdiv-by-zero -Wdouble-promotion "
               "-Wempty-body -Wendif-labels -Wenum-compare -Wextra "
               "-Wfloat-equal -Wformat -Wfree-nonheap-object "
               "-Wignored-qualifiers -Winit-self "
               "-Winline -Wint-to-pointer-cast -Winvalid-memory-model "
               "-Winvalid-offsetof -Wlogical-op -Wmain -Wmaybe-uninitialized "
               "-Wmissing-braces -Wmultichar "
               "-Wnarrowing -Wnoexcept -Wnon-template-friend "
               "-Wnon-virtual-dtor -Wnonnull -Woverflow "
               "-Woverlength-strings -Wparentheses "
               "-Wpmf-conversions -Wpointer-arith -Wreorder "
               "-Wreturn-type -Wsequence-point "
               "-Wsign-compare -Wswitch -Wtype-limits -Wundef "
               "-Wuninitialized -Wunused -Wvla -Wwrite-strings")
  endif()
  # convert CMake list to a single string, erasing the ";" separators
  string(REPLACE ";" "" WFLAGS_STR ${WFLAGS})
  build_string("EXTRA_FLAGS" "${WFLAGS_STR}")
endif()

# Enable ASAN if requested by the user.
if(CAF_ENABLE_ADDRESS_SANITIZER AND NOT WIN32)
  add_compile_options("-fsanitize=address"
                      "-fno-omit-frame-pointer")
  list(APPEND CAF_EXTRA_LDFLAGS "-fsanitize=address")
endif()

# -pthread is ignored on MacOSX but required on other platforms
if(NOT APPLE AND NOT WIN32)
    build_string("EXTRA_FLAGS" "-pthread")
endif()

# -- check if the user provided CXXFLAGS, set defaults otherwise ---------------

if(NOT CMAKE_CXX_FLAGS)
  set(CMAKE_CXX_FLAGS "-Wextra -Wall -pedantic ${EXTRA_FLAGS}")
endif()

if(NOT CMAKE_CXX_FLAGS_DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
endif()

if(NOT CMAKE_CXX_FLAGS_MINSIZEREL)
  set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
endif()

if(NOT CMAKE_CXX_FLAGS_RELEASE)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

if(NOT CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -fno-omit-frame-pointer")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# needed by subprojects
if (DEFINED CMAKE_LD_LIBS)
  list(APPEND ${CMAKE_LD_LIBS})
endif()

# -- find CAF and CAF_NET ------------------------------------------------------

find_package(CAF COMPONENTS core io REQUIRED)
include_directories(${CAF_INCLUDE_DIRS})

find_package(CAF_NET REQUIRED)
include_directories(${CAF_NET_INCLUDE_DIRS})

# -- add targets ---------------------------------------------------------------

# Utility macro for adding benchmark targets
macro(add_target name)
  add_executable(${name}
                "src/${name}.cpp")
  target_link_libraries(${name}
                        ${CAF_EXTRA_LDFLAGS}
                        ${CAF_LIBRARIES}
                        ${CAF_NET_LIBRARIES}
                        ${PTHREAD_LIBRARIES})
endmacro()

add_target(simple_streaming)
add_target(pingpong)
add_target(blank_streaming)
