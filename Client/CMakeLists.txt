 # =======================================================
 # LIB
 # to list all sources to build use:
 #    cd $WK/Client
 #    find src -name \*.cpp --print
 # =======================================================
 
# Excludes src/ClientMain.cpp
list( APPEND srcs
   src/Rtt.cpp
   src/ClientEnvironment.cpp
   src/ClientInvoker.cpp
   src/Rtt.cpp
   src/ClientOptions.cpp
   src/UrlCmd.cpp
)
ecbuild_add_library( TARGET libclient
                     NOINSTALL
                     TYPE     STATIC
                     SOURCES  ${srcs}
                     LIBS     base node nodeattr core 
                              pthread
                     INCLUDES src
                              ../ACore/src 
                              ../ANattr/src
                              ../ANode/src
                              ../Base/src
                              ../Base/src/cts
                              ../Base/src/stc
                )

# ========================================================================
# EXE ecflow_client, if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty

ecbuild_add_executable( TARGET   ecflow_client
                        SOURCES  src/ClientMain.cpp
                        LIBS     libclient ${OPENSSL_LIBRARIES}
                        INCLUDES ${Boost_INCLUDE_DIRS} 
                      )

# Override default behaviour that add RPATHS during install
# The only thing that seem to work is set INSTALL_RPATH to ""
# Using SKIP_BUILD_RPATH,BUILD_WITH_INSTALL_RPATH,INSTALL_RPATH_USE_LINK_PATH
# had no effect
#          
SET_TARGET_PROPERTIES(ecflow_client PROPERTIES 
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


# ================================================================================                         
# TEST
# Use following to populate list: 
#  cd $WK/Client                            
#  find test -name \*.cpp | sort     

list( APPEND test_srcs
    test/TestClientEnvironment.cpp
    test/TestClientInterface.cpp
    test/TestJobGenOnly.cpp
    test/TestLifeCycle.cpp
    test/TestRtt.cpp
    test/TestUrlCmd.cpp
    )

if (ENABLE_SERVER)
    list(APPEND test_srcs
        test/SCPort.cpp
        test/TestClientTimeout.cpp
        test/TestClientHandleCmd.cpp
        test/TestCheckPtDefsCmd.cpp
        test/TestCustomUser.cpp
        test/TestGroupCmd.cpp
        test/TestLoadDefsCmd.cpp
        test/TestPasswdFile.cpp
        test/TestPlugCmd.cpp
        test/TestServer.cpp
        test/TestServerAndLifeCycle.cpp
        test/TestServerLoad.cpp
        test/TestSignalSIGTERM.cpp
        test/TestWhiteListFile.cpp
        )
endif()
              
# if OpenSSL not enabled ${OPENSSL_LIBRARIES}, is empty
ecbuild_add_test( TARGET    s_client
                  SOURCES   ${test_srcs}
                  LIBS      libclient ${OPENSSL_LIBRARIES}
                            ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                  INCLUDES  ../ANode/test
                            ../Base/test 
                            ${Boost_INCLUDE_DIRS}
                  TEST_DEPENDS u_base
                 )


if (ENABLE_ALL_TESTS AND ENABLE_SERVER)
	# 
	# Simple stand alone test
	#
	ecbuild_add_test( TARGET   perf_test_large_defs 
                      SOURCES  test/TestSinglePerf.cpp test/SCPort.cpp
                      LIBS     libclient ${OPENSSL_LIBRARIES}
                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                      INCLUDES ../ANode/test
                               ../Base/test 
                               ${Boost_INCLUDE_DIRS}
                      TEST_DEPENDS u_base
                )
               
	# 
	# test migration
	#
	ecbuild_add_test( TARGET m_test_migration
                      SOURCES  test/TestMigration.cpp test/SCPort.cpp
                      LIBS     libclient ${OPENSSL_LIBRARIES}
                               ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_TEST_EXEC_MONITOR_LIBRARY}
                      INCLUDES ../ANode/test
                               ../Base/test 
                               ${Boost_INCLUDE_DIRS}
                      TEST_DEPENDS u_base
                    )
endif()

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_client DESTINATION bin)
