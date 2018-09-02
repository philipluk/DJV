# Find the GLM library.
#
# This module defines the following variables:
#
# * GLM_FOUND
# * GLM_INCLUDE_DIRS
#
# This module defines the following interfaces:
#
# * GLM

set(GLM_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    GLM
    REQUIRED_VARS GLM_INCLUDE_DIR)
mark_as_advanced(GLM_INCLUDE_DIR)

if(GLM_FOUND AND NOT TARGET GLM)
    add_library(GLM INTERFACE)
    set_target_properties(GLM PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIRS}")
endif()

