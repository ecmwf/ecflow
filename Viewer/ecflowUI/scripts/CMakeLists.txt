#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# the list of files we want to install bin
set(files
  ecflow_ui
)

# the list of files we want to install to share/ecflow/etc
set(files_share
  ecflow_ui_create_jira_issue.sh
  ecflow_ui_node_state_diag.sh
  ecflow_ui_transfer_file.sh
  ecflow_test_ui.sh
)

# set variables which will be expanded in the startup script
if(ENABLE_UI_BACKTRACE)
  set(UI_BACKTRACE yes)
else()
  set(UI_BACKTRACE no)
endif()

if(NOT UI_BACKTRACE_EMAIL_ADDRESS_FILE)
  set(UI_BACKTRACE_EMAIL_ADDRESS_FILE "x")
endif()

if(NOT UI_LOG_FILE)
  set(UI_LOG_FILE "x")
endif()

if(NOT UI_LOG_SITE_TAG)
  set(UI_LOG_SITE_TAG "x")
endif()

# for each file, copy it into the build directory at build time
# and install it into the installation directory at install time (!)
foreach( f ${files} )
  configure_file(${f}.in ${CMAKE_BINARY_DIR}/bin/${f} @ONLY)

  # ensure file is installed at install time
  install(
    FILES
      ${CMAKE_BINARY_DIR}/bin/${f}
    DESTINATION
      bin
    PERMISSIONS
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
      WORLD_READ
      OWNER_EXECUTE
      GROUP_EXECUTE
      WORLD_EXECUTE)
endforeach()


# for each file, copy it into the build directory at build time
# and install it into the installation directory at install time (!)
foreach( f ${files_share} )
  configure_file(${f}.in ${CMAKE_BINARY_DIR}/share/ecflow/etc/${f} @ONLY)

  install(
    FILES
      ${CMAKE_BINARY_DIR}/share/ecflow/etc/${f}
    DESTINATION
      share/ecflow/etc
    PERMISSIONS
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
      WORLD_READ
      OWNER_EXECUTE
      GROUP_EXECUTE
      WORLD_EXECUTE)
endforeach()
