 # =======================================================
 # LIB
 # to list all sources to build use:
 #    cd $WK/Client
 #    find src -name \*.cpp --print
 # =======================================================
 
 # Excludes src/HttpServerMain.cpp
list( APPEND srcs
   src/HttpServer.cpp
   src/ApiV1.cpp
   src/ApiV1Impl.cpp
   src/BasicAuth.cpp
   src/TypeToJson.cpp
   src/TokenStorage.cpp
)
ecbuild_add_library( TARGET libhttp
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                )

target_link_libraries(libhttp base node nodeattr core pthread)
target_include_directories(libhttp PUBLIC src
                                             ../ACore/src
                                             ../ANattr/src
                                             ../ANode/src
                                             ../Base/src
                                             ../Base/src/cts
                                             ../Base/src/stc
					     ../Client/src
                                             ../cpp-httplib
                                             ../json
)



# ========================================================================
# EXE ecflow_http, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable( TARGET   ecflow_http
                        SOURCES  src/HttpMain.cpp
                        LIBS     libhttp libclient ${OPENSSL_LIBRARIES}
                        INCLUDES ${Boost_INCLUDE_DIRS} ../cpp-httplib
                      )

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_http PROPERTIES 
                                    INSTALL_RPATH ""
                     )
                         
# use, i.e. don't skip the full RPATH for the build tree
#SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
#SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

# the RPATH to be used when installing
#SET(CMAKE_INSTALL_RPATH "")

# don't add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
#SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

if (ENABLE_HTTP AND ENABLE_SERVER)

  list(APPEND test_srcs
       test/TestApiV1.cpp
  )

  ecbuild_add_test( TARGET       s_http
                    SOURCES      ${test_srcs}
                    LIBS         libhttp libclient libserver ${OPENSSL_LIBRARIES} 
                                 ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                                 ${Boost_TIMER_LIBRARY} ${Boost_CHRONO_LIBRARY} ${LIBRT}
                    INCLUDES     src
                                 ../ANode/test
                                 ../Base/test
                                 ../Server/src
                                 ${Boost_INCLUDE_DIRS}
                    DEFINITIONS  ${BOOST_TEST_DYN_LINK}
                    TEST_DEPENDS u_base s_client
  )

endif()


# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_http DESTINATION bin)
