# Try to find libcaf_net headers and libraries
#
# Use this module as follows:
#
#     find_package(CAF_NET)
#
# Variables used by this module (they can change the default behavior and need
# to be set before calling find_package):
#
#  CAF_NET_ROOT_DIR Set this variable either to an installation prefix or to
#                   the CAF_NET root directory where to look for the library.
#
# Variables defined by this module:
#
#  CAF_NET_FOUND        Found library and header
#  CAF_NET_LIBRARIES    Path to library
#  CAF_NET_INCLUDE_DIRS Include path for headers
#

# Find libcaf_net header directory
if (CAF_NET_ROOT_DIR)
  set(header_hints
      "${CAF_NET_ROOT_DIR}/include"
      "${CAF_NET_ROOT_DIR}/libcaf_net"
      "${CAF_NET_ROOT_DIR}/../libcaf_net"
      "${CAF_NET_ROOT_DIR}/../../libcaf_net")
endif ()
find_path(CAF_NET_INCLUDE_DIRS
          NAMES
          "caf/net/all.hpp"
          HINTS
          ${header_hints}
          /usr/include
          /usr/local/include
          /opt/local/include
          /sw/include
          ${CMAKE_INSTALL_PREFIX}/include)
mark_as_advanced(CAF_NET_INCLUDE_DIRS)
if (NOT "${CAF_NET_INCLUDE_DIRS}" STREQUAL "CAF_NET_INCLUDE_DIRS-NOTFOUND")
  # signal headers found
  set(CAF_NET_FOUND true)
  # check for CMake-generated export header for the net component
  find_path(net_export_header_path
            NAMES
            caf/detail/net_export.hpp
            HINTS
            ${CAF_NET_ROOT_DIR}
            ${header_hints}
            /usr/include
            /usr/local/include
            /opt/local/include
            /sw/include
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})
  if ("${net_export_header_path}" STREQUAL "net_export_header_path-NOTFOUND")
    message(WARNING "Found all.hpp for CAF net, but not net_export.hpp")
    set(CAF_NET_FOUND false)
  else ()
    list(APPEND CAF_NET_INCLUDE_DIRS "${net_export_header_path}")
  endif ()
  # now try to find libcaf_net library
  if (CAF_NET_ROOT_DIR)
    set(library_hints "${CAF_NET_ROOT_DIR}/libcaf_net")
  endif ()
  find_library(CAF_NET_LIBRARIES
               NAMES
               "caf_net"
               "caf_net_static"
               HINTS
               ${library_hints}
               /usr/lib
               /usr/local/lib
               /opt/local/lib
               /sw/lib
               ${CMAKE_INSTALL_PREFIX}/lib)
  mark_as_advanced(CAF_NET_LIBRARIES)
  if ("${CAF_NET_LIBRARIES}" STREQUAL "CAF_NET_LIBRARIES-NOTFOUND")
    set(CAF_NET_FOUND false)
  endif ()
endif ()
# let CMake check whether all requested components have been found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CAF_NET
                                  FOUND_VAR CAF_NET_FOUND
                                  REQUIRED_VARS CAF_NET_LIBRARIES CAF_NET_INCLUDE_DIRS
                                  HANDLE_COMPONENTS)
# final step to tell CMake we're done
mark_as_advanced(CAF_NET_ROOT_DIR
                 CAF_NET_LIBRARIES
                 CAF_NET_INCLUDE_DIRS)
