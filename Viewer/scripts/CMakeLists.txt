
# the list of files we want to install
set (files ecflow_ui)

# for each file, copy it into the build directory at build time
# and install it into the installation directory at install time (!)
foreach( f ${files} )
    configure_file(${f} ${CMAKE_BINARY_DIR}/bin COPYONLY)

    # copy to the build 'bin' directory, setting exe permissions
    file(COPY ${f}
         DESTINATION ${CMAKE_BINARY_DIR}/bin
         FILE_PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ OWNER_EXECUTE GROUP_EXECUTE
        )

    #message(STATUS ${f} ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_BINARY_DIR} ${CMAKE_BINARY_DIR})

    # ensure file is installed at install time
    install( FILES ${CMAKE_BINARY_DIR}/bin/${f}
             DESTINATION bin
             PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)


endforeach()
