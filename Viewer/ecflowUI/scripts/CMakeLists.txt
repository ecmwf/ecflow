
# the list of files we want to install
set (files ecflow_ui ecflow_test_ui.sh node_state_diag.sh)


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
    install( FILES ${CMAKE_BINARY_DIR}/bin/${f}
             DESTINATION bin
             PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)

endforeach()
