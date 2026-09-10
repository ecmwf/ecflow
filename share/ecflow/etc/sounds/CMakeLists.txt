# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

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
