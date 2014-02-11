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
                   LIBS     core nodeattr node libparser base ${Ecflow_Boost_LIBS}
                   INCLUDES ../ACore/src 
                            ../ANattr/src
                            ../ANode/src
                            ../AParser/src
                            ../Base/src
                            ../Base/src/cts
                            ../Base/src/stc
                            ${Boost_INCLUDE_DIRS} )

# ========================================================================
# EXE ecflow_client

ecbuild_add_executable( TARGET   ecflow_client
                        SOURCES  src/ClientMain.cpp
                        LIBS     core nodeattr node libparser base libclient pthread ${Ecflow_Boost_LIBS}
                        INCLUDES src
                                 ${Boost_INCLUDE_DIRS} 
                     )
                          
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
ecbuild_add_test( TARGET  s_client
                SOURCES   ${test_srcs}
                LIBS      core nodeattr node libparser base libclient pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ../Base/src/cts
                          ../Base/src/stc
                          ../Base/test 
                          ../Client/src
                          ${Boost_INCLUDE_DIRS} )

# 
# Simple stand alone test
#
ecbuild_add_test( TARGET   perf_test_large_defs 
                SOURCES  test/TestSinglePerf.cpp test/SCPort.cpp
                LIBS     core nodeattr node libparser base libclient pthread ${Ecflow_Test_LIBS}
                INCLUDES  src
                          ../ACore/src 
                          ../ANattr/src
                          ../ANode/src
                          ../ANode/test
                          ../Base/src
                          ../Base/src/cts
                          ../Base/src/stc
                          ../Base/test 
                          ../Client/src
                          ${Boost_INCLUDE_DIRS} 
               )
               
# 
# test migration
#
ecbuild_add_test( TARGET test_migration
                SOURCES  test/TestMigration.cpp test/SCPort.cpp
                LIBS     core nodeattr node libparser base libclient pthread ${Ecflow_Test_LIBS}
                INCLUDES src
                         ../ACore/src 
                         ../ANattr/src
                         ../ANode/src
                         ../ANode/test
                         ../Base/src
                         ../Base/src/cts
                         ../Base/src/stc
                         ../Base/test 
                         ../Client/src
                         ${Boost_INCLUDE_DIRS} 
               )

# ===================================================================
# install
# ===================================================================
install (TARGETS ecflow_client DESTINATION bin)
