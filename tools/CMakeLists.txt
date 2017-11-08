#
# Configure  ecflow_site.sh. This is used in ecflow_start.sh and ecflow_stop.sh
# Used to coping with start of ecflow server on linux cluser
# Avoid hard copy mention of paths specific to ECMWF

if( DEFINED SITE_SPECIFIC_SERVER_SCRIPT )

	macro(install_symlink filepath sympath)
    	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})")
    	install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
	endmacro(install_symlink)
	
    install_symlink(${SITE_SPECIFIC_SERVER_SCRIPT} ${CMAKE_INSTALL_PREFIX}/bin/ecflow_site.sh )

endif()

install( FILES
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.pl
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.sh
            ${CMAKE_SOURCE_DIR}/tools/noconnect.sh
            ${CMAKE_SOURCE_DIR}/tools/ecflow_start.sh
            ${CMAKE_SOURCE_DIR}/tools/ecflow_stop.sh
         DESTINATION bin
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
         COMPONENT binary
       )