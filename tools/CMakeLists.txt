#
# Configure exflow_start.sh and ecflow_stop.sh
#   For ECMWF start/stop server has hostsfile, to cope with linux clusters
# Use to avoid hard copy mention of paths specific to ECMWF

if( DEFINED ECMWF_SITE_SPECIFIC )
	set( ECMWF_SITE_SPECIFIC_FILE ${ECMWF_SITE_SPECIFIC} )
else()
	set( ECMWF_SITE_SPECIFIC_FILE "/path/to/file/that/should/not_exist/ecmwf_site_specfic_file")
endif()

configure_file( ecflow_start.sh.in ecflow_start.sh @ONLY) # @ONLY -> use @VAR@ for substitution
configure_file( ecflow_stop.sh.in  ecflow_stop.sh  @ONLY) # @ONLY -> use @VAR@ for substitution

install( FILES
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.pl
            ${CMAKE_SOURCE_DIR}/tools/ecflow_logsvr.sh
            ${CMAKE_SOURCE_DIR}/tools/noconnect.sh
            ${CMAKE_CURRENT_BINARY_DIR}/ecflow_start.sh
            ${CMAKE_CURRENT_BINARY_DIR}/ecflow_stop.sh
         DESTINATION bin
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
         COMPONENT binary
       )