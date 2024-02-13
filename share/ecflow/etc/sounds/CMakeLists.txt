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

# for each file, copy it into the build directory at build time
# and install it into the installation directory at install time (!)
foreach( f ${files} )
  configure_file(${f} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
    
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${f}
    DESTINATION
      share/ecflow/etc/sounds
    PERMISSIONS
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
      WORLD_READ
  )
endforeach()
