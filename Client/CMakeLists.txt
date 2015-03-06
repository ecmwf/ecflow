 # =======================================================
 # LIB
 # to list all sources to build use:
 #    cd $WK/Base
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
                   LIBS     base libparser node nodeattr core 
                            pthread
                   INCLUDES src
                            ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ../Base/src/cts
                            ../Base/src/stc
                )

# ========================================================================
# EXE ecflow_client
ecbuild_add_executable( TARGET   ecflow_client
                        SOURCES  src/ClientMain.cpp
                        LIBS     libclient 
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
 test/SCPort.cpp
 test/TestCheckPtDefsCmd.cpp
 test/TestClientEnvironment.cpp
 test/TestClientInterface.cpp
 test/TestClientInvoker.cpp
 test/TestClientTimeout.cpp
 test/TestGroupCmd.cpp
 test/TestJobGenOnly.cpp
 test/TestLifeCycle.cpp
 test/TestLoadDefsCmd.cpp
 test/TestPlugCmd.cpp
 test/TestRtt.cpp
 test/TestServer.cpp
 test/TestServerAndLifeCycle.cpp
 test/TestSignalSIGTERM.cpp
 test/TestUrlCmd.cpp
 test/TestWhiteListFile.cpp
)
ecbuild_add_test( TARGET    s_client
                  BOOST
                  SOURCES   ${test_srcs}
                  LIBS      libclient 
                  INCLUDES  ../ANode/test
                            ../Base/test 
                  TEST_DEPENDS u_base
                 )

# 
# Simple stand alone test
#
ecbuild_add_test( TARGET   perf_test_large_defs 
                  BOOST
                  SOURCES  test/TestSinglePerf.cpp test/SCPort.cpp
                  LIBS     libclient 
                  INCLUDES ../ANode/test
                           ../Base/test 
                  TEST_DEPENDS u_base
                )
               
# 
# test migration
#
ecbuild_add_test( TARGET test_migration
                  BOOST
                  SOURCES  test/TestMigration.cpp test/SCPort.cpp
                  LIBS     libclient 
                  INCLUDES ../ANode/test
                           ../Base/test 
                  TEST_DEPENDS u_base
                )

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_client DESTINATION bin)
