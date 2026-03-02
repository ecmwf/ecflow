#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# the files we want to install
set (files
  tone.ogg
)

set(DST share/ecflow/etc/sounds)
set(DIR ${CMAKE_BINARY_DIR}/${DST})

file(MAKE_DIRECTORY ${DIR})

# for each file, copy it into the build directory at build time
# and install it into the installation directory at install time (!)
foreach( f ${files} )
  configure_file(${f} ${DIR} COPYONLY)

  if (NOT EXISTS ${DIR}/${f})
    message(FATAL_ERROR "File ${DIR}/${f} does not exist after configuration")
  endif()

  install(
    FILES
      ${DIR}/${f}
    DESTINATION
      ${DST}
    PERMISSIONS
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
      WORLD_READ
  )
endforeach()
