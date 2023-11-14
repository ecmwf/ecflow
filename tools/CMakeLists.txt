#
# Copyright 2023- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This can be used by submission system. i.e trimurti
ecbuild_add_executable(
  TARGET
    ecflow_standalone
  SOURCES
    ecflow_standalone.c
)

#
# Configure  ecflow_site.sh. This is used in ecflow_start.sh and ecflow_stop.sh
# Used in coping with start of ecflow server on a linux cluser
# Avoid's hard copy mention of paths specific to ECMWF

if( DEFINED SITE_SPECIFIC_SERVER_SCRIPT )
  macro(install_symlink filepath sympath)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})")
    install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
  endmacro(install_symlink)
  
  install_symlink(${SITE_SPECIFIC_SERVER_SCRIPT} ${CMAKE_INSTALL_PREFIX}/bin/ecflow_site.sh )
endif()

install(
  FILES
    ${CMAKE_SOURCE_DIR}/tools/ecflow_fuse.py
    ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.pl
    ${CMAKE_SOURCE_DIR}/tools/ecflow_start.sh
    ${CMAKE_SOURCE_DIR}/tools/ecflow_stop.sh
    ${CMAKE_SOURCE_DIR}/tools/ecflow_logserver.sh
  DESTINATION bin
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE
  COMPONENT
    binary
)
